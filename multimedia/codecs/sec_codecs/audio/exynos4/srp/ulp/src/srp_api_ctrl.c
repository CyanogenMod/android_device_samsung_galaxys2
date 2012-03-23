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

#include "srp_api_ctrl.h"
#include "srp_ioctl.h"

#define LOG_TAG "libsrpapi"
#include <cutils/log.h>

/* Disable LOGD message */
#ifdef LOGD
#undef LOGD
#endif
#define LOGD(...)

static int srp_ctrl = -1;
static int srp_ctrl_cnt = 0;
static short pcm_buf[2048]; /* 4KBytes data, 1K frames (16bit stereo data) */

#ifdef _USE_FW_FROM_DISK_
static char srp_alt_fw_name_pre[6][32] = {
    "sdcard/rp_fw/rp_fw_code1",
    "sdcard/rp_fw/rp_fw_code20",
    "sdcard/rp_fw/rp_fw_code21",
    "sdcard/rp_fw/rp_fw_code22",
    "sdcard/rp_fw/rp_fw_code30",
    "sdcard/rp_fw/rp_fw_code31",
};
#endif

static int SRP_Ctrl_Open(void)
{
    if (srp_ctrl_cnt == 0) {
        srp_ctrl = open(SRP_CTRL_DEV_NAME, O_RDWR | O_NDELAY);
        if (srp_ctrl < 0) {
            LOGE("%s: Failed open device file %d", __func__, srp_ctrl);
            return -1;
        }
        srp_ctrl_cnt++;
        LOGV("%s: Device is opened[%d]: cnt %d", __func__, srp_ctrl, srp_ctrl_cnt);
    }

    return srp_ctrl;
}

static int SRP_Ctrl_Close(void)
{
    int ret = 0;

    if (srp_ctrl_cnt == 1) {
        ret = close(srp_ctrl);
        if (ret < 0) {
            LOGE("%s: Failed closen device file %d", __func__, srp_ctrl);
            return -1;
        }
        srp_ctrl_cnt--;
        LOGV("%s: Device is closed[%d]: cnt %d", __func__, srp_ctrl, srp_ctrl_cnt);
        srp_ctrl = -1;
    }

    return ret;
}

#ifdef _USE_FW_FROM_DISK_
/* This will check & download alternate firmware */
static int SRP_Check_AltFirmware(void)
{
    unsigned long *temp_buff;
    FILE *fp = NULL;

    char alt_fw_name[128];
    unsigned long alt_fw_set;
    unsigned long alt_fw_loaded = 0;
    int alt_fw_text_ok,alt_fw_data_ok;

    if ((srp_ctrl = SRP_Ctrl_Open()) >= 0) {
        ioctl(srp_ctrl, SRP_CTRL_ALTFW_STATE, &alt_fw_loaded);

        if (!alt_fw_loaded) {    /* Not loaded yet? */
            LOGE("Try to download alternate RP firmware");
            temp_buff = (unsigned long *)malloc(256*1024);    /* temp buffer */

            for (alt_fw_set = 0; alt_fw_set < 6; alt_fw_set++) {
                sprintf(alt_fw_name, "%s_text.bin", srp_alt_fw_name_pre[alt_fw_set]);
                if (fp = fopen(alt_fw_name, "rb")) {
                    LOGE("RP Alt-Firmware Loading: %s", alt_fw_name);
                    fread(temp_buff, 64*1024, 1, fp);
                    close(fp);
                    alt_fw_text_ok = 1;
                } else {
                    alt_fw_text_ok = 0;
                }

                sprintf(alt_fw_name, "%s_data.bin", srp_alt_fw_name_pre[alt_fw_set]);
                if (fp = fopen(alt_fw_name, "rb")) {
                    LOGE("RP Alt-Firmware Loading: %s", alt_fw_name);
                    fread(&temp_buff[64*1024/4], 96*1024, 1, fp);
                    close(fp);
                    alt_fw_data_ok = 1;
                } else {
                    alt_fw_data_ok = 0;
                }

                if (alt_fw_text_ok && alt_fw_data_ok) {
                    temp_buff[160*1024/4] = alt_fw_set;
                    ioctl(srp_ctrl, SRP_CTRL_ALTFW_LOAD, temp_buff);
                }
            }
            free(temp_buff);
        }
        SRP_Ctrl_Close();
    }

    return 0;
}
#endif

int SRP_Ctrl_Set_Effect(int effect)
{
    int ret;
    unsigned long effect_mode = (unsigned long)effect;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_SET_EFFECT, effect_mode);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Enable_Effect(int on)
{
    int ret;
    unsigned long effect_switch = on ? 1 : 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_EFFECT_ENABLE, effect_switch);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Set_Effect_Def(unsigned long effect_def)
{
    int ret;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_EFFECT_DEF, effect_def);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Set_Effect_EQ_User(unsigned long eq_user)
{
    int ret;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_EFFECT_EQ_USR, eq_user);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Set_Pcm_Dump(int on)
{
    int ret;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_PCM_DUMP_OP, on);

    LOGV("dump_op: %d", on);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Get_Pcm_Dump_State(void)
{
    int ret;
    int srp_dump_stat = 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_IS_PCM_DUMP, &srp_dump_stat);

    LOGV("srp_dump_stat: %d", srp_dump_stat);

    SRP_Ctrl_Close();

    return srp_dump_stat;
}

int SRP_Ctrl_Set_Gain(float value)
{
    int ret;
    unsigned long gain = 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    gain = (unsigned long)((1 << 24) * value);
    ioctl(srp_ctrl, SRP_CTRL_SET_GAIN, gain);

    SRP_Ctrl_Close();

    return 0;
}

int SRP_Ctrl_Get_Running_Stat(void)
{
    int ret;
    int srp_running_stat = 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_IS_RUNNING, &srp_running_stat);

    LOGV("srp_running_stat: %d", srp_running_stat);

    SRP_Ctrl_Close();

    return srp_running_stat;
}

int SRP_Ctrl_Get_Open_Stat(void)
{
    int ret;
    int srp_open_stat = 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return -1;
    }

    ioctl(srp_ctrl, SRP_CTRL_IS_OPENED, &srp_open_stat);

    LOGV("srp_open_stat: %d", srp_open_stat);

    SRP_Ctrl_Close();

    return srp_open_stat;
}

short *SRP_Ctrl_Get_Pcm(void)
{
    int ret;
    int rp_is_running = 0;
    int dump_is_on = 0;
    int rp_is_opened = 0;

    ret = SRP_Ctrl_Open();
    if (ret < 0) {
        LOGE("%s: SRP_Ctrl_Open error", __func__);
        return NULL;
    }

    ioctl(srp_ctrl, SRP_CTRL_IS_RUNNING, &rp_is_running);
    if (rp_is_running) {
        ioctl(srp_ctrl, SRP_CTRL_IS_PCM_DUMP, &dump_is_on);
        if (dump_is_on == 0) {
            ioctl(srp_ctrl, SRP_CTRL_PCM_DUMP_OP, 1);
            dump_is_on = 1;
        }

        ioctl(srp_ctrl, SRP_CTRL_GET_PCM_1KFRAME, pcm_buf);
        return pcm_buf;
    }

    /* SRP is not running */
    if (srp_ctrl > 0) {
        if (dump_is_on) {
            ioctl(srp_ctrl, SRP_CTRL_IS_OPENED, &rp_is_opened);
            if (rp_is_opened)
                ioctl(srp_ctrl, SRP_CTRL_PCM_DUMP_OP, 0);
        }
        SRP_Ctrl_Close();
    }

    return NULL;
}
