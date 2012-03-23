/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      SecFimc.h
 * \brief     header file for Fimc HAL MODULE
 * \author    Hyunkyung, Kim(hk310.kim@samsung.com)
 * \date      2010/10/13
 *
 * <b>Revision History: </b>
 * - 2010/10/13 : Hyunkyung, Kim(hk310.kim@samsung.com) \n
 *   Initial version
 *
 * - 2011/11/15 : Sunmi, Lee(carrotsm.lee@samsung.com) \n
 *   Adjust V4L2 architecture \n
 */

#ifndef __SEC_FIMC_H__
#define __SEC_FIMC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/fb.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <asm/sizes.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <hardware/hardware.h>

#include "utils/Timers.h"

#ifdef BOARD_USE_V4L2
#include "s5p_fimc_v4l2.h"
#include "sec_utils_v4l2.h"
#else
#include "s5p_fimc.h"
#include "sec_utils.h"
#endif
#include "sec_format.h"

#include "SecBuffer.h"
#include "SecRect.h"

#define PFX_NODE_FIMC        "/dev/video"
#define MAX_DST_BUFFERS     (3)
#define MAX_SRC_BUFFERS     (1)
#define MAX_PLANES          (3)

#ifdef __cplusplus
}

class SecFimc
{
public:
    enum DEV {
        DEV_0 = 0,
        DEV_1,
        DEV_2,
        DEV_3,
        DEV_MAX,
    };

    enum MODE {
        MODE_NONE = 0,
        MODE_SINGLE_BUF,
        MODE_MULTI_BUF,
        MODE_DMA_AUTO,
        MODE_MAX,
    };

private:
    bool                        mFlagCreate;
    int                         mDev;
    int                         mFimcMode;
    int                         mNumOfBuf;

    int                         mRealDev;
    int                         mFd;
    int                         mHwVersion;
    int                         mRotVal;
    bool                        mFlagGlobalAlpha;
    int                         mGlobalAlpha;
    bool                        mFlagLocalAlpha;
    bool                        mFlagColorKey;
    int                         mColorKey;
    bool                        mFlagSetSrcParam;
    bool                        mFlagSetDstParam;
    bool                        mFlagStreamOn;

    s5p_fimc_t                  mS5pFimc;
    struct v4l2_capability      mFimcCap;

    SecBuffer                   mSrcBuffer;
    SecBuffer                   mDstBuffer[MAX_DST_BUFFERS];

public:
    SecFimc();
    virtual ~SecFimc();

    virtual bool create(enum DEV dev, enum MODE mode, int numOfBuf);
    virtual bool destroy(void);
    bool flagCreate(void);

    int  getFd(void);

    SecBuffer * getMemAddr(int index = 0);

    int  getHWVersion(void);

    virtual bool setSrcParams(unsigned int width, unsigned int height,
                      unsigned int cropX, unsigned int cropY,
                      unsigned int *cropWidth, unsigned int *cropHeight,
                      int colorFormat,
                      bool forceChange = true);

    virtual bool getSrcParams(unsigned int *width, unsigned int *height,
                      unsigned int *cropX, unsigned int *cropY,
                      unsigned int *cropWidth, unsigned int *cropHeight,
                      int *colorFormat);

    virtual bool setSrcAddr(unsigned int physYAddr,
                    unsigned int physCbAddr = 0,
                    unsigned int physCrAddr = 0,
                    int colorFormat = 0);

    virtual bool setDstParams(unsigned int width, unsigned int height,
                      unsigned int cropX, unsigned int cropY,
                      unsigned int *cropWidth, unsigned int *cropHeight,
                      int colorFormat,
                      bool forceChange = true);

    virtual bool getDstParams(unsigned int *width, unsigned int *height,
                      unsigned int *cropX, unsigned int *cropY,
                      unsigned int *cropWidth, unsigned int *cropHeight,
                      int *colorFormat);

    virtual bool setDstAddr(unsigned int physYAddr, unsigned int physCbAddr = 0, unsigned int physCrAddr = 0, int buf_index = 0);

    virtual bool setRotVal(unsigned int rotVal);
    virtual bool setGlobalAlpha(bool enable = true, int alpha = 0xff);
    virtual bool setLocalAlpha(bool enable);
    virtual bool setColorKey(bool enable = true, int colorKey = 0xff);

    virtual bool draw(int src_index, int dst_index);

private:
    bool m_streamOn(void);
    bool m_checkSrcSize(unsigned int width, unsigned int height,
                        unsigned int cropX, unsigned int cropY,
                        unsigned int *cropWidth, unsigned int *cropHeight,
                        int colorFormat,
                        bool forceChange = false);

    bool m_checkDstSize(unsigned int width, unsigned int height,
                        unsigned int cropX, unsigned int cropY,
                        unsigned int *cropWidth, unsigned int *cropHeight,
                        int colorFormat,
                        int rotVal,
                        bool forceChange = false);
    int  m_widthOfFimc(int v4l2ColorFormat, int width);
    int  m_heightOfFimc(int v4l2ColorFormat, int height);
    int  m_getYuvBpp(unsigned int fmt);
    int  m_getYuvPlanes(unsigned int fmt);
};
#endif

#endif //__SEC_FIMC_H__
