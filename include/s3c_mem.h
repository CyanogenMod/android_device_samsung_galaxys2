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

#ifndef _S3C_MEM_COMMON_H_
#define _S3C_MEM_COMMON_H_

#define MEM_IOCTL_MAGIC         'M'

#define S3C_MEM_ALLOC           _IOWR(MEM_IOCTL_MAGIC, 310, struct s3c_mem_alloc)
#define S3C_MEM_FREE            _IOWR(MEM_IOCTL_MAGIC, 311, struct s3c_mem_alloc)

#define S3C_MEM_SHARE_ALLOC     _IOWR(MEM_IOCTL_MAGIC, 314, struct s3c_mem_alloc)
#define S3C_MEM_SHARE_FREE      _IOWR(MEM_IOCTL_MAGIC, 315, struct s3c_mem_alloc)

#define S3C_MEM_CACHEABLE_ALLOC     _IOWR(MEM_IOCTL_MAGIC, 316, struct s3c_mem_alloc)
#define S3C_MEM_CACHEABLE_SHARE_ALLOC   _IOWR(MEM_IOCTL_MAGIC, 317, struct s3c_mem_alloc)

#define S3C_MEM_CACHE_FLUSH     _IOWR(MEM_IOCTL_MAGIC, 318, struct s3c_mem_alloc)
#define S3C_MEM_CACHE_INVAL     _IOWR(MEM_IOCTL_MAGIC, 319, struct s3c_mem_alloc)
#define S3C_MEM_CACHE_CLEAN     _IOWR(MEM_IOCTL_MAGIC, 320, struct s3c_mem_alloc)

struct s3c_mem_alloc {
    int     size;
    unsigned int    vir_addr;
    unsigned int    phy_addr;
};

struct s3c_mem_dma_param {
    int     size;
    unsigned int    src_addr;
    unsigned int    dst_addr;
    int     cfg;
};

#endif // _S3C_MEM_COMMON_H_
