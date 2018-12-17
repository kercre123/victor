/* Copyright (c) 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdlib.h>

#include "ion_mem.h"
#include <linux/msm_ion.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <linux/types.h>

/* Allocate ION memory */
void mem_alloc_ion(const clst_test_env env, struct mmap_info_ion * pmapion, size_t alignment_in_bytes, int cpu_cache_policy)
{
	struct ion_handle_data handle_data;
	struct ion_allocation_data alloc;
	struct ion_client *ion_client;
	struct ion_handle *ion_handle;
	int32_t rc;
	int32_t ion_fd;
	void * ret;
	int32_t  p_pmem_fd;
	struct ion_fd_data ion_info_fd;
	struct ion_fd_data test_fd;
	struct ion_custom_data data;

	ion_fd = open("/dev/ion", O_RDONLY);
	if (ion_fd < 0)
	{
		if(!env->output_csv)
		{
			printf("mem_alloc_ion : Open ion device failed\n");
		}
		pmapion->pVirtualAddr = NULL;
		return;
	}

	alloc.len = pmapion->bufsize;
	alloc.align = alignment_in_bytes;

#ifdef ANDROID
	alloc.heap_id_mask = ( 0x1 << ION_IOMMU_HEAP_ID);
#else
	alloc.heap_id_mask = ( 0x1 << ION_IOMMU_HEAP_ID);
#endif

	if(cpu_cache_policy == CL_MEM_HOST_WRITEBACK_QCOM)
		alloc.flags = ION_FLAG_CACHED;
	else if(cpu_cache_policy == CL_MEM_HOST_UNCACHED_QCOM)
		alloc.flags= 0;
	else {
		printf("Invalid CPU cache policy specified. Please use CL_MEM_HOST_WRITEBACK_QCOM or CL_MEM_HOST_UNCACHED_QCOM\n");
		return;
	}

	rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc);

	if (rc < 0)
	{
		handle_data.handle =  (pmapion->ion_info_fd).handle;
		if(!env->output_csv)
		{
			printf("mem_alloc_ion : ION alloc length %d %d\n", alloc.len, errno);
		}
		close(ion_fd);
		pmapion->pVirtualAddr = NULL;
		return;
	}
	else
	{
		pmapion->ion_info_fd.handle = alloc.handle;
		rc = ioctl(ion_fd, ION_IOC_SHARE, &(pmapion->ion_info_fd));

		if (rc < 0)
		{
			if(!env->output_csv)
			{
				printf("mem_alloc_ion : ION map call failed %d\n",rc);
			}
			handle_data.handle = pmapion->ion_info_fd.handle;
			ioctl(ion_fd, ION_IOC_FREE, &handle_data);
			close(ion_fd);
			pmapion->pVirtualAddr = NULL;
			return;
		}
		else
		{
			p_pmem_fd = pmapion->ion_info_fd.fd;
			ret = mmap(NULL, alloc.len,
					PROT_READ  | PROT_WRITE,
					MAP_SHARED,
					p_pmem_fd, 0);
			if (ret == MAP_FAILED)
			{
				if(!env->output_csv)
				{
					printf("mem_alloc_ion : mmap call failed %d\n", errno);
				}
				handle_data.handle = (pmapion->ion_info_fd).handle;
				ioctl(ion_fd, ION_IOC_FREE, &handle_data);
				close(ion_fd);
				pmapion->pVirtualAddr = NULL;
				return;
			}
			else
			{
#ifdef DEBUG
				if(!env->output_csv)
				{
					printf("Ion allocation success! hostptr : 0x%x\n",(uint32_t) ret);
				}
#endif
				data.cmd = p_pmem_fd;
				pmapion->pVirtualAddr = ret;
				pmapion->ion_fd = ion_fd;
				pmapion->pPhyAddr= (uint8_t**) data.arg;
			}
		}
	}
	return;
}

/* Free ION Memory */

void mem_free_ion(struct mmap_info_ion mapion)
{
	int32_t rc = 0;
	struct ion_handle_data handle_data;
	uint32_t bufsize;

	bufsize = (mapion.bufsize + 4095) & (~4095);
	rc = munmap(mapion.pVirtualAddr, bufsize);
	close(mapion.ion_info_fd.fd);

	handle_data.handle = mapion.ion_info_fd.handle;
	ioctl(mapion.ion_fd, ION_IOC_FREE, &handle_data);
	close(mapion.ion_fd);
}
