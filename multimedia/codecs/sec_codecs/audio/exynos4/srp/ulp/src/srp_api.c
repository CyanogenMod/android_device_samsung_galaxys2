#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "srp_api.h"

#define LOG_TAG "libsrpapi"
#include <cutils/log.h>

/* Disable LOGD message */
#ifdef LOGD
#undef LOGD
#endif
#define LOGD(...)

//#define _USE_WBUF_            /* Buffering before writing srp-rp device */
//#define _DUMP_TO_FILE_
//#define _USE_FW_FROM_DISK_

#ifdef _USE_WBUF_
#define WBUF_LEN_MUL        2
#endif

static int srp_dev = -1;
static int srp_ibuf_size = 0;
static int srp_block_mode = SRP_INIT_BLOCK_MODE;

static unsigned char *wbuf;
static int wbuf_size;
static int wbuf_pos;

#ifdef _DUMP_TO_FILE_
static FILE *fp_dump = NULL;
#endif

#ifdef _USE_WBUF_
static int WriteBuff_Init(void)
{
    if (wbuf == NULL) {
        wbuf_size = srp_ibuf_size * WBUF_LEN_MUL;
        wbuf_pos = 0;
        wbuf = (unsigned char *)malloc(wbuf_size);
        LOGD("%s: WriteBuffer %dbytes allocated", __func__, wbuf_size);
        return 0;
    }

    LOGE("%s: WriteBuffer already allocated", __func__);
    return -1;
}

static int WriteBuff_Deinit(void)
{
    if (wbuf != NULL) {
        free(wbuf);
        wbuf = NULL;
        return 0;
    }

    LOGE("%s: WriteBuffer is not ready", __func__);
    return -1;
}

static int WriteBuff_Write(unsigned char *buff, int size_byte)
{
    int write_byte;

    if ((wbuf_pos + size_byte) < wbuf_size) {
        memcpy(&wbuf[wbuf_pos], buff, size_byte);
        wbuf_pos += size_byte;
    } else {
        LOGE("%s: WriteBuffer is filled [%d], ignoring write [%d]", __func__, wbuf_pos, size_byte);
        return -1;    /* Insufficient buffer */
    }

    return wbuf_pos;
}

static void WriteBuff_Consume(void)
{
    memcpy(wbuf, &wbuf[srp_ibuf_size], srp_ibuf_size * (WBUF_LEN_MUL - 1));
    wbuf_pos -= srp_ibuf_size;
}

static void WriteBuff_Flush(void)
{
    wbuf_pos = 0;
}
#endif

int SRP_Create(int block_mode)
{
    if (srp_dev == -1) {
#ifdef _USE_FW_FROM_DISK_
        SRP_Check_AltFirmware();
#endif

        srp_block_mode = block_mode;
        srp_dev = open(SRP_DEV_NAME, O_RDWR |
                    ((block_mode == SRP_INIT_NONBLOCK_MODE) ? O_NDELAY : 0));

        return srp_dev;
    }

    LOGE("%s: Device is not ready", __func__);
    return -1;    /* device alreay opened */
}

int SRP_Init(unsigned int ibuf_size)
{
    int ret;

    if (srp_dev != -1) {
        srp_ibuf_size = ibuf_size;
        ret = ioctl(srp_dev, SRP_INIT, srp_ibuf_size); /* Initialize IBUF size (4KB ~ 18KB) */

#ifdef _DUMP_TO_FILE_
        char outname[256];
        int cnt = 0;

        while (1) {
            sprintf(outname, "/data/rp_dump_%04d.mp3", cnt++);
            if (fp_dump = fopen(outname, "rb")) { /* file exist? */
                fclose(fp_dump);
            } else {
                break;
            }
        }

        LOGD("%s: Dump MP3 to %s", __func__, outname);
        if (fp_dump = fopen(outname, "wb"))
            LOGD("%s: Success to open %s", __func__, outname);
        else
            LOGD("%s: Fail to open %s", __func__, outname);
#endif

#ifdef _USE_WBUF_
        if (ret != -1)
            return WriteBuff_Init();
#else
        return ret;
#endif
    }

    LOGE("%s: Device is not ready", __func__);
    return -1;  /* device is not created */
}

#ifdef _USE_WBUF_
int SRP_Decode(void *buff, int size_byte)
{
    int ret;
    int val;
    int err_code = 0;

    if (srp_dev != -1) {
        /* Check wbuf before writing buff */
        while (wbuf_pos >= srp_ibuf_size) { /* Write_Buffer filled? (IBUF Size)*/
            LOGD("%s: Write Buffer is full, Send data to RP", __func__);

            ret = write(srp_dev, wbuf, srp_ibuf_size); /* Write Buffer to RP Driver */
            if (ret == -1) { /* Fail? */
                ioctl(srp_dev, SRP_ERROR_STATE, &val);
                if (!val) {    /* Write error? */
                    LOGE("%s: IBUF write fail", __func__);
                    return -1;
                } else {       /* Write OK, but RP decode error? */
                    err_code = val;
                    LOGE("%s: RP decode error [0x%05X]", __func__, err_code);
                }
            }
#ifdef _DUMP_TO_FILE_
            if (fp_dump)
                fwrite(wbuf, srp_ibuf_size, 1, fp_dump);
#endif
            WriteBuff_Consume();
        }

        ret = WriteBuff_Write((unsigned char *)buff, size_byte);
        if (ret == -1)
            return -1;  /* Buffering error */

        LOGD("%s: Write Buffer remain [%d]", __func__, wbuf_pos);
        return err_code;  /* Write Success */
    }

    LOGE("%s: Device is not ready", __func__);
    return -1;  /* device is not created */
}

int SRP_Send_EOS(void)
{
    int ret;
    int val;

    if (srp_dev != -1) {
        /* Check wbuf before writing buff */
        while (wbuf_pos) { /* Write_Buffer ramain?*/
            if (wbuf_pos < srp_ibuf_size) {
                memset(wbuf + wbuf_pos, 0xFF, srp_ibuf_size - wbuf_pos); /* Fill dummy data */
                wbuf_pos = srp_ibuf_size;
            }

            ret = write(srp_dev, wbuf, srp_ibuf_size); /* Write Buffer to RP Driver */
            if (ret == -1) {  /* Fail? */
                ret = ioctl(srp_dev, SRP_ERROR_STATE, &val);
                if (!val) {   /* Write error? */
                    LOGE("%s: IBUF write fail", __func__);
                    return -1;
                } else {      /* RP decoe error? */
                    LOGE("%s: RP decode error [0x%05X]", __func__, val);
                    return -1;
                }
            } else {          /* Success? */
#ifdef _DUMP_TO_FILE_
                if (fp_dump)
                    fwrite(wbuf, srp_ibuf_size, 1, fp_dump);
#endif
                WriteBuff_Consume();
            }
        }

        memset(wbuf, 0xFF, srp_ibuf_size);      /* Fill dummy data */
        write(srp_dev, wbuf, srp_ibuf_size); /* Write Buffer to RP Driver */

        /* Wait until RP decoding over */
        return ioctl(srp_dev, SRP_WAIT_EOS);
    }

    return -1; /* device is not created */
}
#else  /* Without WBUF */
int SRP_Decode(void *buff, int size_byte)
{
    int ret;
    int val;
    int err_code = 0;

    if (srp_dev != -1) {
        LOGD("%s: Send data to RP (%d bytes)", __func__, size_byte);

        ret = write(srp_dev, buff, size_byte);  /* Write Buffer to RP Driver */
        if (ret == -1) {  /* Fail? */
            ioctl(srp_dev, SRP_ERROR_STATE, &val);
            if (!val) {   /* Write error? */
                LOGE("%s: IBUF write fail", __func__);
                return -1;
            } else {      /* Write OK, but RP decode error? */
                err_code = val;
                LOGE("%s: RP decode error [0x%05X]", __func__, err_code);
            }
        }
#ifdef _DUMP_TO_FILE_
        if (fp_dump)
            fwrite(buff, size_byte, 1, fp_dump);
#endif

        return err_code; /* Write Success */
    }

    LOGE("%s: Device is not ready", __func__);
    return -1; /* device is not created */
}

int SRP_Send_EOS(void)
{
    /* Wait until RP decoding over */
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_SEND_EOS);

    return -1; /* device is not created */
}

int SRP_Resume_EOS(void)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_RESUME_EOS);

    return -1; /* device is not created */
}
#endif

int SRP_Pause(void)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_PAUSE);

    return -1; /* device is not created */
}

int SRP_Stop(void)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_STOP);

    return -1; /* device is not created */
}

int SRP_Flush(void)
{
    if (srp_dev != -1) {
        if (ioctl(srp_dev, SRP_FLUSH) != -1) {
#ifdef _USE_WBUF_
            WriteBuff_Flush();
#endif
            return 0;
        }
    }

    return -1; /* device is not created */
}


int SRP_SetParams(int id, unsigned long val)
{
    if (srp_dev != -1)
        return 0; /* not yet */

    return -1;    /* device is not created */
}

int SRP_GetParams(int id, unsigned long *pval)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, id, pval);

    return -1;    /* device is not created */
}

int SRP_Deinit(void)
{
    if (srp_dev != -1) {
#ifdef _DUMP_TO_FILE_
        if (fp_dump)
            fclose(fp_dump);
#endif

#ifdef _USE_WBUF_
        WriteBuff_Deinit();
#endif
        return ioctl(srp_dev, SRP_DEINIT); /* Deinialize */
    }

    LOGE("%s: Device is not ready", __func__);
    return -1;    /* device is not created */
}

int SRP_Terminate(void)
{
    int ret;

    if (srp_dev != -1) {
        ret = close(srp_dev);

        if (ret == 0) {
            srp_dev = -1; /* device closed */
            return 0;
        }
    }

    LOGE("%s: Device is not ready", __func__);
    return -1; /* device is not created or close error*/
}

int SRP_IsOpen(void)
{
    if (srp_dev == -1) {
        LOGD("%s: Device is not opened", __func__);
        return 0;
    }

    LOGD("%s: Device is opened", __func__);
    return 1;
}
