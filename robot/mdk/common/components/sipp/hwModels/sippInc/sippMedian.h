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

#ifndef __SIPP_MEDIAN_H__
#define __SIPP_MEDIAN_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE       (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

const char med_kernel_sizes[] = "357";

typedef struct {
	int kernel_size;
	int out_sel;
	int threshold;
} MedianParameters;

class MedianFilt : public SippBaseFilt {

public:
	MedianFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 7,
		std::string name = "Median filter");
	~MedianFilt() {
	}

	void SetKernelSize(int, int reg = 0);
	void SetOutputSelection(int, int reg = 0);
	void SetThreshold(int, int reg = 0);

	int GetKernelSize(int reg = 0)      { return med_params[reg].kernel_size; };
	int GetOutputSelection(int reg = 0) { return med_params[reg].out_sel; };
	int GetThreshold(int reg = 0)       { return med_params[reg].threshold; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);
	
	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	MedianParameters med_params[2];
	int kernel_size;
	int out_sel;
	int threshold;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

// Comparison function for qsort()
int fcompare(const void *, const void *);

#endif // __SIPP_MEDIAN_H__
