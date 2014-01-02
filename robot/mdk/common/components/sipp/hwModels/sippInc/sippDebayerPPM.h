// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Debayer post-processing median filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_DBYR_PPM_H__
#define __SIPP_DBYR_PPM_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE       (kernel_lines >> 1) * (kernel_lines) + (kernel_lines >> 1)
#endif

typedef struct {
	int data_width;
} DebayerPPMParameters;

class DebayerPPMFilt : public SippBaseFilt {

public:
	DebayerPPMFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		std::string name = "Debayer PPM filter");
	~DebayerPPMFilt() {
	}

	void SetDataWidth(int, int reg = 0);

	void ComputeDifferences(uint16_t *, uint16_t *, int32_t *);

	int GetDataWidth(int reg = 0) { return dbyrppm_params[reg].data_width; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	DebayerPPMParameters dbyrppm_params[2];
	int kernel_size;
	int data_width;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the Debayer post-processing median filter
	void Run(void ***, void **);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

// Comparison function for qsort()
int compare(const void *, const void *);

#endif // __SIPP_DBYR_PPM_H__
