// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model - Median filter
//
// 2D Median filter, center pixel of pixel kernel is replaced with median
// value for pixel kernel. Pixel kernel size is configurable from 3x3 up to
// 15x15 (odd dimensions only). Only pixels above a programmable threshold level
// are filtered.
// -----------------------------------------------------------------------------

#include "sippMedian.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>


MedianFilt::MedianFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k),
	threshold (-1),
	out_sel (24) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
}

void MedianFilt::SetKernelSize(int val, int reg) {
	if(!strchr(med_kernel_sizes, (char)(val + '0'))) {
		std::cerr << "Invalid kernel size: possible sizes are 3, 5 or 7." << std::endl;
		abort();
	} else {
 		med_params[reg].kernel_size = val;
		base_params[reg].max_pad = val >> 1;
	}
}

void MedianFilt::SetOutputSelection(int val, int reg) {
	if(val < 0 || val > med_params[reg].kernel_size * med_params[reg].kernel_size - 1) {
		std::cerr << "Output selection exceeds kernel's boundaries! Must be in range [0, " << med_params[reg].kernel_size * med_params[reg].kernel_size - 1 << "]" << std::endl;
		abort();
	}
	med_params[reg].out_sel = val;
}

void MedianFilt::SetThreshold(int val, int reg) {
	// Threshold is signed 9 bit: sign extend register bits into int storage
	med_params[reg].threshold = (((val & 0x1ff) << ((sizeof(int)*8) - 9)) >> ((sizeof(int)*8) - 9));
}

void MedianFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	kernel_size = med_params[reg].kernel_size;
	out_sel     = med_params[reg].out_sel;
	threshold   = med_params[reg].threshold;
}

void MedianFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr = 0;
	uint8_t **      in_ptr = 0;
	uint8_t *      out_ptr = 0;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	split_in_ptr = new uint8_t *[kernel_size];
	      in_ptr = new uint8_t *[kernel_size];
	for(int ln = 0; ln < kernel_size; ln++)
		in_ptr[ln] = new uint8_t[width];

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Filter a line from each plane
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Prepare filter buffer line pointers then run
		getKernelLines(split_in_ptr, kernel_size, pl);
		PackInputLines(split_in_ptr, kernel_size, in_ptr);
		setOutputLine(&out_ptr, pl);

		Run((void **)in_ptr, (void *)out_ptr);
	}
	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	delete[] split_in_ptr;
	delete[]       in_ptr;
}

void MedianFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr = 0;
	uint8_t **      in_ptr = 0;
	uint8_t *      out_ptr = 0;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	split_in_ptr = new uint8_t *[kernel_size];
	      in_ptr = new uint8_t *[kernel_size];
	for(int ln = 0; ln < kernel_size; ln++)
		in_ptr[ln] = new uint8_t[width];

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			getKernelLines(split_in_ptr, kernel_size, pl);
			PackInputLines(split_in_ptr, kernel_size, in_ptr);
			setOutputLine(&out_ptr, pl);

			Run((void **)in_ptr, (void *)out_ptr);
		}
		// Update buffer fill levels and input frame line index
		UpdateBuffers();
		IncLineIdx();

		// At the end of the frame
		if (EndOfFrame()) {
			// Trigger end of frame interrupt
			EndOfFrameIrq();

			// Disable the filter unless the
			// input buffer is circular
			if(in.GetBuffLines() == 0) {
				Disable();
				break;
			}
		}
	}
	cs.Leave();

	delete[] split_in_ptr;
	delete[]       in_ptr;
}

void *MedianFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t **      src = 0;
	uint8_t *    kernel = 0;
	uint8_t *       dst = 0;
	uint8_t *packed_dst = 0;

	// Allocating array of pointers to input lines
	src = new uint8_t *[kernel_size];

	// Setting input pointers
	for (int i = 0; i < kernel_size; i++)
		src[i] = (uint8_t *)in_ptr[i];

	// 2D kernel
	kernel = new uint8_t[kernel_size*kernel_size];

	// Setting output pointer
	dst = (uint8_t *)out_ptr;

	// Allocate temporary packed output buffer
	packed_dst = new uint8_t[width];

	int x = 0;

	while (x < width) {
		// Build the 2D kernel
		buildKernel(src, kernel, kernel_size, x);

		// Sort the kernel if the centre pixel is greater than the threshold
		if (int(kernel[KERNEL_CENTRE]) > threshold) {
			qsort((void *)kernel, (size_t)(kernel_size*kernel_size), sizeof(uint8_t), fcompare);

			// Pick the filtered value
			packed_dst[x] = kernel[out_sel];
		} else {
			// Pass through
			packed_dst[x] = kernel[KERNEL_CENTRE];
		}
		x++;
	}

	SplitOutputLine(packed_dst, width, dst);

	delete[] kernel;
	delete[] packed_dst;
	delete[] src;

	return out_ptr;
}

// Comparison function for qsort()
int fcompare(const void *a, const void *b) {
	uint8_t *pa = (uint8_t *)a;
	uint8_t *pb = (uint8_t *)b;
	return *pa == *pb ? 0 : *pa < *pb ? -1 : 1;
}
