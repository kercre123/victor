/* Copyright (c) 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef ION_MEM_H
#define ION_MEM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <linux/msm_ion.h>
#include "clstlib.h"

typedef struct mmap_info
{
	int32_t 	pmem_fd;
	uint8_t* 	pVirtualAddr;
	uint8_t* 	pPhyAddr;
	uint32_t 	map_buf_size;
	uint32_t 	filled_len;
} mmap_info;

typedef struct mmap_info_ion
{
	int32_t 	ion_fd;
	uint8_t*   *pVirtualAddr;
	uint8_t*   *pPhyAddr;
	size_t      bufsize;
	struct ion_fd_data ion_info_fd;
} mmap_info_ion;


void mem_alloc_ion(const clst_test_env env, struct mmap_info_ion * pmapion, size_t alignment_in_bytes, int cpu_cache_policy);

void mem_free_ion(struct mmap_info_ion mapion);

#endif /*ION_MEM_H*/
