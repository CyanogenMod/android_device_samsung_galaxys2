#ifndef __SRP_API_H__
#define __SRP_API_H__

#include "srp_ioctl.h"
#include "srp_error.h"

#define SRP_DEV_NAME                    "dev/srp"

#define SRP_INIT_BLOCK_MODE             0
#define SRP_INIT_NONBLOCK_MODE          1

#define SRP_PENDING_STATE_RUNNING       0
#define SRP_PENDING_STATE_PENDING       1

struct srp_buf_info {
    void *mmapped_addr;
    void *addr;
    unsigned int mmapped_size;
    unsigned int size;
    int num;
};

struct srp_dec_info {
    unsigned int sample_rate;
    unsigned int channels;
};

#ifdef __cplusplus
extern "C" {
#endif

int SRP_Create(int block_mode);
int SRP_Init();
int SRP_Decode(void *buff, int size_byte);
int SRP_Send_EOS(void);
int SRP_SetParams(int id, unsigned long val);
int SRP_GetParams(int id, unsigned long *pval);
int SRP_Deinit(void);
int SRP_Terminate(void);
int SRP_IsOpen(void);

int SRP_Get_Ibuf_Info(void **addr, unsigned int *size, unsigned int *num);
int SRP_Get_Obuf_Info(void **addr, unsigned int *size, unsigned int *num);
int SRP_Get_Dec_Info(struct srp_dec_info *dec_info);
int SRP_Get_PCM(void **addr, unsigned int *size);
int SRP_Flush(void);

#ifdef __cplusplus
}
#endif

#endif /*__SRP_API_H__ */
