/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef HARDWARE_CONVERTER_H_

#define HARDWARE_CONVERTER_H_

#include <OMX_Video.h>

class HardwareConverter {
public:
    HardwareConverter();
    ~HardwareConverter();
    bool convert(
        void * src_addr,
        void * dst_addr,
        OMX_COLOR_FORMATTYPE src_format,
        int32_t width,
        int32_t height,
        OMX_COLOR_FORMATTYPE dst_format);
    bool bHWconvert_flag;
private:
    void *mSecFimc;
    unsigned int OMXtoHarPixelFomrat(OMX_COLOR_FORMATTYPE omx_format);
};

void test_function();

#endif  // HARDWARE_CONVERTER_H_
