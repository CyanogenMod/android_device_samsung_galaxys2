/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Portions of this code have been modified from the original.
 * These modifications are:
 *    * includes
 *    * struct private_handle_t
 *    * usesPhysicallyContiguousMemory()
 *    * validate()
 *    * dynamicCast()
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>

/*#include <ump/ump.h>*/
#include "ump.h"

/*
 * HWC_HWOVERLAY is flag for location of glReadPixels().
 * Enable this define if you want that glReadPixesl() is in HWComposer.
 * If you disable this define, glReadPixesl() is called in threadloop().
 */
#define HWC_HWOVERLAY 1

#define GRALLOC_ARM_UMP_MODULE 1

enum {
#ifndef SAMSUNG_CODEC_SUPPORT
    GRALLOC_USAGE_HW_FIMC1              = 0x01000000,
    GRALLOC_USAGE_HW_ION                = 0x02000000,
    GRALLOC_USAGE_YUV_ADDR              = 0x04000000,

    /* SEC Private usage , for Overlay path at HWC */
    GRALLOC_USAGE_HWC_HWOVERLAY         = 0x20000000,
#endif

    /* SEC Private usage , for HWC to set HDMI S3D format */
    /* HDMI should display this buffer as S3D SBS LR/RL*/
    GRALLOC_USAGE_PRIVATE_SBS_LR        = 0x00400000,
    GRALLOC_USAGE_PRIVATE_SBS_RL        = 0x00200000,

    /* HDMI should display this buffer as 3D TB LR/RL*/
    GRALLOC_USAGE_PRIVATE_TB_LR         = 0x00100000,
    GRALLOC_USAGE_PRIVATE_TB_RL         = 0x00080000,
};

struct private_handle_t;

struct private_module_t {
    gralloc_module_t base;

    private_handle_t* framebuffer;
    uint32_t flags;
    uint32_t numBuffers;
    uint32_t bufferMask;
    pthread_mutex_t lock;
    buffer_handle_t currentBuffer;
    int ion_client;

    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
    float xdpi;
    float ydpi;
    float fps;
    int enableVSync;

    enum {
        PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
    };
};

#ifdef USE_PARTIAL_FLUSH
struct private_handle_rect {
    int handle;
    int stride;
    int l;
    int t;
    int w;
    int h;
    int locked;
    struct private_handle_rect *next;
};
#endif

#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t {
    struct native_handle nativeHandle;
#endif
    enum {
        PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
        PRIV_FLAGS_USES_UMP    = 0x00000002,
        PRIV_FLAGS_USES_PMEM   = 0x00000004,
        PRIV_FLAGS_USES_IOCTL  = 0x00000008,
        PRIV_FLAGS_USES_HDMI   = 0x00000010,
        PRIV_FLAGS_USES_ION    = 0x00000020,
        PRIV_FLAGS_NONE_CACHED = 0x00000040,
    };

    enum {
        LOCK_STATE_WRITE     =   1<<31,
        LOCK_STATE_MAPPED    =   1<<30,
        LOCK_STATE_READ_MASK =   0x3FFFFFFF
    };

    int     fd;

    int     magic;
    int     flags;
    int     size;
    int     base;
    int     lockState;
    int     writeOwner;
    int     pid;

    /* Following members are for UMP memory only */
    int     ump_id;
    int     ump_mem_handle;
    int     offset;
    int     paddr;

    int     format;
    int     usage;
    int     width;
    int     height;
    int     bpp;
    int     stride;

    /* Following members are for ION memory only */
    int     ion_client;

    /* Following members ard for YUV information */
    unsigned int yaddr;
    unsigned int uoffset;
    unsigned int voffset;

#ifdef __cplusplus
    static const int sNumInts = 21;
    static const int sNumFds = 1;
    static const int sMagic = 0x3141592;

    private_handle_t(int flags, int size, int base, int lock_state, ump_secure_id secure_id, ump_handle handle,int fd_val, int offset_val, int paddr_val):
    fd(fd_val),
    magic(sMagic),
    flags(flags),
    size(size),
    base(base),
    lockState(lock_state),
    writeOwner(0),
    pid(getpid()),
    ump_id((int)secure_id),
    ump_mem_handle((int)handle),
    offset(offset_val),
    paddr(paddr_val),
    format(0),
    usage(0),
    width(0),
    height(0),
    bpp(0),
    stride(0),
    ion_client(0),
    yaddr(0),
    uoffset(0),
    voffset(0)
    {
        version = sizeof(native_handle);
        numFds = sNumFds;
        numInts = sNumInts;
    }

    private_handle_t(int flags, int size, int base, int lock_state, int fb_file, int fb_offset):
    fd(fb_file),
    magic(sMagic),
    flags(flags),
    size(size),
    base(base),
    lockState(lock_state),
    writeOwner(0),
    pid(getpid()),
    ump_id((int)UMP_INVALID_SECURE_ID),
    ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
    offset(fb_offset),
    paddr(0),
    format(0),
    usage(0),
    width(0),
    height(0),
    bpp(0),
    stride(0),
    ion_client(0),
    yaddr(0),
    uoffset(0),
    voffset(0)
    {
        version = sizeof(native_handle);
        numFds = sNumFds;
        numInts = sNumInts;
    }

    ~private_handle_t()
    {
        magic = 0;
    }

    bool usesPhysicallyContiguousMemory()
    {
        return (flags & PRIV_FLAGS_FRAMEBUFFER) ? true : false;
    }

    static int validate(const native_handle* h)
    {
        const private_handle_t* hnd = (const private_handle_t*)h;
        if (!h || h->version != sizeof(native_handle) ||
            h->numInts != sNumInts ||
            h->numFds != sNumFds ||
            hnd->magic != sMagic)
            return -EINVAL;
        return 0;
    }

    static private_handle_t* dynamicCast(const native_handle* in)
    {
        if (validate(in) == 0)
            return (private_handle_t*) in;
        return NULL;
    }
#endif
};

#endif /* GRALLOC_PRIV_H_ */
