// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Sharpen filter
//
// 2D Sharpening filter: unsharp mask is created by subtracting the Gaussian blur 
// filter output from the original pixels. If the absolute value of the mask is
// greater than a threshold, it is multiplied by a strength factor and added back
// to the input pixels in order to give a sharpened output. Pixel kernel size is 
// configurable from 3x3 up to 7x7 (odd dimensions only). Sharpening can be bypassed
// and the blurred image output instead.
// -----------------------------------------------------------------------------

#ifndef __SIPP_SHARPEN_H__
#define __SIPP_SHARPEN_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>


#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE        (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

// Sharpen filter modes
#define BLUR                  1
#define SHARPEN               0

const char sharpen_kernel_sizes[] = "357";

const int smooth_kernel[] = {0, 1, 0, 1, 4, 1, 0, 1, 0};

typedef struct {
	int kernel_size;
	int output_clamp;
	int mode;
	int output_deltas;
	fp16 threshold;
	fp16 strength_pos;
	fp16 strength_neg;
	fp16 clipping_alpha;
	fp16 undershoot;
	fp16 overshoot;
	fp16 range_stops[4];
	fp16 blur_coeffs[7];
} SharpenParameters;

class SharpenFilt : public SippBaseFilt {

public:
	SharpenFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 7,
		std::string name = "Sharpening filter");
	~SharpenFilt() {
	}

	void SetKernelSize(int, int reg = 0);
	void SetOutputClamp(int, int reg = 0);
	void SetMode(int, int reg = 0);
	void SetOutputDeltas(int, int reg = 0);
	void SetThreshold(int, int reg = 0);
	void SetPosStrength(int, int reg = 0);
	void SetNegStrength(int, int reg = 0);
	void SetClippingAlpha(int, int reg = 0);
	void SetUndershoot(int, int reg = 0);
	void SetOvershoot(int, int reg = 0);
	void SetRngstop0(int, int reg = 0);
	void SetRngstop1(int, int reg = 0);
	void SetRngstop2(int, int reg = 0);
	void SetRngstop3(int, int reg = 0);
	void SetCoeff0(int, int reg = 0);
	void SetCoeff1(int, int reg = 0);
	void SetCoeff2(int, int reg = 0);
	void SetCoeff3(int, int reg = 0);

	void AutoCompleteArray(void);
	float GetSmoothPixel(fp16 *, int);
	void  BuildMMR(fp16 *, float *, int);
	void  GetMinMax(float *, fp16 *, fp16 *);

	int GetKernelSize(int reg = 0)    { return kernel_size; };
	int GetOutputClamp(int reg = 0)   { return output_clamp; };
	int GetMode(int reg = 0)          { return mode; };
	int GetOutputDeltas(int reg = 0)  { return output_deltas; };
	int GetThreshold(int reg = 0)     { return threshold.getPackedValue(); };	
	int GetPosStrength(int reg = 0)   { return strength_pos.getPackedValue(); };
	int GetNegStrength(int reg = 0)   { return strength_neg.getPackedValue(); };
	int GetClippingAlpha(int reg = 0) { return clipping_alpha.getPackedValue(); };
	int GetUndershoot(int reg = 0)    { return undershoot.getPackedValue(); };
	int GetOvershoot(int reg = 0)     { return overshoot.getPackedValue(); };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	SharpenParameters usm_params[2];
	int kernel_size;
	int output_clamp;
	int mode;
	int output_deltas;
	fp16 threshold;
	fp16 strength_pos;
	fp16 strength_neg;
	fp16 clipping_alpha;
	fp16 undershoot;
	fp16 overshoot;
	fp16 range_stops[4];
	fp16 blur_coeffs[7];
	int coeff_offset;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_SHARPEN_H__
