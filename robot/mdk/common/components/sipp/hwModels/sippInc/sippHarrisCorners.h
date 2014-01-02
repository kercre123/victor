// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Harris corners filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_HARRIS_H__
#define __SIPP_HARRIS_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE       (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

#define RADIUS              3
#define FP16_TO_FP32_SCALE  24

const char harris_kernel_sizes[] = "579";

const int sub_kernel[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};

typedef struct {
	int kernel_size;
	float k;
} HarrisCornersParameters;

class HarrisCornersFilt : public SippBaseFilt {

public:
	HarrisCornersFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 9,
		std::string name = "Harris corners filter");
	~HarrisCornersFilt() {
	}

	void SetKernelSize(int, int reg = 0);
	void SetK(int, int reg = 0);

	int DxConvolution(uint8_t *, int, int, int);
	int DyConvolution(uint8_t *, int, int, int);
	float Sum(float *, int);

	int GetKernelSize(int reg = 0) { return harr_params[reg].kernel_size; };
	int GetK(int reg = 0)          { return *(int *)&harr_params[reg].k; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	HarrisCornersParameters harr_params[2];
	int kernel_size;
	float k;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_HARRIS_H__
