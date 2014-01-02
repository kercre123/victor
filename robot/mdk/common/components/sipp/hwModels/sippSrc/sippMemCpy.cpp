// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model - MemCpy filter
//
// The implementations of Execute() and Run() here may be used as as templates/
// starting points for actual filter implementations.
//
// -----------------------------------------------------------------------------

#include "sippMemCpy.h"

#include <iostream>
#include <string>


MemCpyFilt::MemCpyFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

	// pObj, name, id and kernel_lines are protected members of SippBaseFilter
	SetPIrqObj (pObj);
	SetName (name);
	SetId (id);
	SetKernelLinesNo(kernel_size);
}

void *MemCpyFilt::Run(void **in_ptr, void *out_ptr) {
	fp16 *src;
	fp16 *dst;

	static int count = 0;

	if (Verbose())
		std::cout << name << " run " << count++ << std::endl;

	// memcpy(out_ptr, in_ptr[0], width * sizeof(fp16));
	src = (fp16 *)in_ptr[0];
	dst = (fp16 *)out_ptr;

	int x = 0;
	while (x++ < width) {
		*dst++ = *src++;
	}

	return out_ptr;
}
