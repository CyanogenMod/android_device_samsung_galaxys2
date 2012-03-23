#ifndef __SRP_API_H__
#define __SRP_API_H__

#include "srp_ioctl.h"

#ifdef __cplusplus
extern "C" {
#endif

int SRP_Create(int block_mode);
int SRP_Init(unsigned int ibuf_size);
int SRP_Decode(void *buff, int size_byte);
int SRP_Send_EOS(void);
int SRP_Resume_EOS(void);
int SRP_Pause(void);
int SRP_Stop(void);
int SRP_Flush(void);
int SRP_SetParams(int id, unsigned long val);
int SRP_GetParams(int id, unsigned long *pval);
int SRP_Deinit(void);
int SRP_Terminate(void);
int SRP_IsOpen(void);

#define SRP_DEV_NAME                         "dev/srp"

#define SRP_INIT_BLOCK_MODE                  0
#define SRP_INIT_NONBLOCK_MODE               1

#define SRP_PENDING_STATE_RUNNING            0
#define SRP_PENDING_STATE_PENDING            1

#define SRP_ERROR_LOSTSYNC                   0x00101
#define SRP_ERROR_BADLAYER                   0x00102
#define SRP_ERROR_BADBITRATE                 0x00103
#define SRP_ERROR_BADSAMPLERATE              0x00104
#define SRP_ERROR_BADEMPHASIS                0x00105

#define SRP_ERROR_BADCRC                     0x00201
#define SRP_ERROR_BADBITALLOC                0x00211
#define SRP_ERROR_BADBADSCALEFACTOR          0x00221
#define SRP_ERROR_BADFRAMELEN                0x00231
#define SRP_ERROR_BADBIGVALUES               0x00232
#define SRP_ERROR_BADBLOCKTYPE               0x00233
#define SRP_ERROR_BADSCFSI                   0x00234
#define SRP_ERROR_BADDATAPTR                 0x00235
#define SRP_ERROR_BADPART3LEN                0x00236
#define SRP_ERROR_BADHUFFTABLE               0x00237
#define SRP_ERROR_BADHUFFDATA                0x00238
#define SRP_ERROR_BADSTEREO                  0x00239

#ifdef __cplusplus
}
#endif

#endif /*__SRP_API_H__ */
