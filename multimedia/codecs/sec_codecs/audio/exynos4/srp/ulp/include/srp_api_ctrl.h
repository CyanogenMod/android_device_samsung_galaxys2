#ifndef __SRP_API_CTRL_H__
#define __SRP_API_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SRP_CTRL_DEV_NAME    "dev/srp_ctrl"

int SRP_Ctrl_Set_Effect(int effect);    /* test only */
int SRP_Ctrl_Enable_Effect(int on);
int SRP_Ctrl_Set_Effect_Def(unsigned long effect_def);
int SRP_Ctrl_Set_Effect_EQ_User(unsigned long eq_user);
int SRP_Ctrl_Set_Pcm_Dump(int on);
int SRP_Ctrl_Get_Pcm_Dump_State(void);
int SRP_Ctrl_Set_Gain(float value);
int SRP_Ctrl_Get_Running_Stat(void);
int SRP_Ctrl_Get_Open_Stat(void);
short *SRP_Ctrl_Get_Pcm(void);

#ifdef __cplusplus
}
#endif

#endif /* __SRP_API_CTRL_H__ */
