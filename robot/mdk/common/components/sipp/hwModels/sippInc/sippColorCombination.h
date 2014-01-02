// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Color combination filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_CC_H__
#define __SIPP_CC_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

// Phase = -1/4
const char kern1[] = {-1, 15, 55, -5};
// Phase =  1/4
const char kern2[] = {-5, 55, 15, -1};

class ColorCombinationFilt : public SippBaseFilt {

public:
	Buffer chr;

	ColorCombinationFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		int ck = 0,
		std::string name = "Color combination filter");
	~ColorCombinationFilt() {
	}

	void SetForceLumaOne(int);
	void SetMulCoeff(int);
	void SetThreshold(int);
	void SetPlaneMultiple(int);
	void SetRPlaneCoeff(int);
	void SetGPlaneCoeff(int);
	void SetBPlaneCoeff(int);
	void SetEpsilon(int);
	void SetCCM00(int);
	void SetCCM01(int);
	void SetCCM02(int);
	void SetCCM10(int);
	void SetCCM11(int);
	void SetCCM12(int);
	void SetCCM20(int);
	void SetCCM21(int);
	void SetCCM22(int);

	void AdjustDimensions(void);

	int GetForceLumaOne()  { return force_luma_one; };
	int GetMulCoeff()      { return mul; };
	int GetThreshold()     { return t1; };
	int GetPlaneMultiple() { return plane_multiple; };
	int GetRPlaneCoeff()   { return k_rgb[0].full; };
	int GetGPlaneCoeff()   { return k_rgb[1].full; };
	int GetBPlaneCoeff()   { return k_rgb[2].full; };
	int GetEpsilon()       { return epsilon; };

	int  IncInputBufferFillLevel (void);
	bool CanRunCc(int, int);
	void getChromaLines (uint8_t ***);
	void PackChromaLines(uint8_t ***, int, uint8_t ***);
	int  NextChromaSlice(int offset = 0);
	void ComputeChromaStartSlice(int);
	void PadAtTop(uint8_t ***, int);
	void PadAtBottom(uint8_t ***, int);
	void MiddleExecutionV(uint8_t ***);
	void UpdateBuffers(void);
	void UpdateBuffersSync(void);
	void UpdateChromaBuffer(void);
	void UpdateChromaBufferSync(void);
	void UpdateChromaBufferNoWrap(void);
	int  IncChrLineIdx(void);

	void buildChromaKernel(int *, int *, int);
	void PadAtLeft(int *, int *, int, int);
	void PadAtRight(int *, int *, int, int);
	void MiddleExecutionH(int *, int *, int);

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	int kernel_size;
	int chr_kernel_size;
	int chr_plane_start_slice;
	int force_luma_one;
	uint8_t mul;
	uint8_t t1;
	int plane_multiple;
	FIXED4_8 k_rgb[3];
	uint16_t epsilon;
	FIXED6_10 ccm[9];
	char even_coeffs[4];
	char  odd_coeffs[4];
	uint16_t crt_conv_luma;
	uint16_t conv_t1;
	int chr_width;
	int chr_height;
	int chr_min_fill;
	int chr_max_pad;
	int chr_line_idx;
	int chr_inc_en;
	int luma_used_bits;
	int luma_mask;
	int luma_shift_amount;
	int full_set;
	int remainder;
	int crt_output_planes;
	int crt_output_sp;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the Color combination filter
	void *Run(void **, void ***, void **);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_CC_H__
