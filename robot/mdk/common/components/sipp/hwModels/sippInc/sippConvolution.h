// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Programmable convolution filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_CONV_H__
#define __SIPP_CONV_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>


#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE       (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

const char conv_kernel_sizes[] = "35";

typedef struct {
	int kernel_size;
	int output_clamp;
	int abs_bit;
	int sq_bit;
	int accum_bit;
	int od_bit;
	fp16 thr;
	fp16 coeffs[25];
} ConvolutionParameters;

class ConvolutionFilt : public SippBaseFilt {

public:
	ConvolutionFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 5,
		std::string name = "Convolution filter");
	~ConvolutionFilt() {
	}

	void SetKernelSize(int, int reg = 0);
	void SetOutputClamp(int, int reg = 0);
	void SetAbsBit(int, int reg = 0);
	void SetSqBit(int, int reg = 0);
	void SetAccumBit(int, int reg = 0);
	void SetOdBit(int, int reg = 0);
	void SetThreshold(int, int reg = 0);
	void SetCoeff00(int, int reg = 0);
	void SetCoeff01(int, int reg = 0);
	void SetCoeff02(int, int reg = 0);
	void SetCoeff03(int, int reg = 0);
	void SetCoeff04(int, int reg = 0);
	void SetCoeff10(int, int reg = 0);
	void SetCoeff11(int, int reg = 0);
	void SetCoeff12(int, int reg = 0);
	void SetCoeff13(int, int reg = 0);
	void SetCoeff14(int, int reg = 0);
	void SetCoeff20(int, int reg = 0);
	void SetCoeff21(int, int reg = 0);
	void SetCoeff22(int, int reg = 0);
	void SetCoeff23(int, int reg = 0);
	void SetCoeff24(int, int reg = 0);
	void SetCoeff30(int, int reg = 0);
	void SetCoeff31(int, int reg = 0);
	void SetCoeff32(int, int reg = 0);
	void SetCoeff33(int, int reg = 0);
	void SetCoeff34(int, int reg = 0);
	void SetCoeff40(int, int reg = 0);
	void SetCoeff41(int, int reg = 0);
	void SetCoeff42(int, int reg = 0);
	void SetCoeff43(int, int reg = 0);
	void SetCoeff44(int, int reg = 0);

	void UpdateBuffers(void);
	void UpdateBuffersSync(void);

	int GetKernelSize(int reg = 0)	   { return conv_params[reg].kernel_size; };
	int GetOutputClamp(int reg = 0)    { return conv_params[reg].output_clamp; };
	int GetAbsBit(int reg = 0)		   { return conv_params[reg].abs_bit; };
	int GetSqBit(int reg = 0)		   { return conv_params[reg].sq_bit; };
	int GetAccumBit(int reg = 0)	   { return conv_params[reg].accum_bit; };
	int GetOdBit(int reg = 0)		   { return conv_params[reg].od_bit; };
	int GetThreshold(int reg = 0)	   { return conv_params[reg].thr.getPackedValue(); };
	int GetAccumResult()               { return (int &)accum_result; }; // Type pun float to int
	int GetAccumCnt()                  { return accum_cnt_result; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	ConvolutionParameters conv_params[2];
	int kernel_size;
	int output_clamp;
	int abs_bit;
	int sq_bit;
	int accum_bit;
	int od_bit;
	fp16 thr;
	float        accum;
	float        accum_result;
	unsigned int accum_cnt;
	unsigned int accum_cnt_result;
	fp16 coeffs[25];

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_CONV_H__
