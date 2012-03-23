#include <jni.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "srp_api_ctrl.h"

#define LOG_TAG "libsa_jni"
#include <cutils/log.h>

void Java_com_android_music_SetSACtrlJNI_set(JNIEnv * env, jobject obj, int effect_num)
{
    unsigned long effect_enable = effect_num ? 1 : 0;
    unsigned int ret;

    LOGD("Sound effect[%d]", effect_num);

    ret = SRP_Ctrl_Enable_Effect(effect_enable);
    if (ret < 0) {
        LOGE("%s: Couldn't enabled effect\n", __func__);
        return;
    }

    SRP_Ctrl_Set_Effect_Def(effect_num << 5);
    if (ret < 0) {
        LOGE("%s: Couldn't defined effect\n", __func__);
        return;
    }

    return;
}
