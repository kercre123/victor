// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model - MemCpy filter
//
// This class is used as a placeholder for filters which have not yet been
// implemented. Data is read from the input buffer and copied to the output
// buffer unmodified, a la memcpy().
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_MEMCPY_H__
#define __SIPP_MEMCPY_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>


class MemCpyFilt : public SippBaseFilt {

public:
	MemCpyFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 1,
		std::string name = "MemCpy filter");
	~MemCpyFilt() {
	}

private:
	int kernel_size;

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_MEMCPY_H__
