// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Debayer filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_DBYR_H__
#define __SIPP_DBYR_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>


// Bilinear demosaicing starting point
#define BIL_SP               (ahd_kernel_size >> 1)
// Green channel interpolation starting point
#define GI_SP                2
// Red/blue channel interpolation starting point
#define RBI_SP               (GI_SP + 1)
// Homogeneity maps starting point
#define HM_SP                1
// Choice starting point
#define CHC_SP               (HM_SP + 1)
// Homogeneity maps lines at a step
#define HM_LINES             (ahd_kernel_size - 2 * RBI_SP - 2 * HM_SP)

#define GENERATE_MATRIX_COEFFICIENTS  0
#define DONT_ROUND                    0

const double xyz_rgb[3][3] = {          /* XYZ from RGB */
  { 0.412453, 0.357580, 0.180423 },
  { 0.212671, 0.715160, 0.072169 },
  { 0.019334, 0.119193, 0.950227 } };

const float rgb_cam[3][4] = {			/* XYZ from RGB */
  { 1, 0, 0, 0 },
  { 0, 1, 0, 0 },
  { 0, 0, 1, 0 },
};

const uint8_t luma_bayer[] = {1, 2, 1, /* Local luma from Bayer */
                              2, 4, 2,
                              1, 2, 1
};

const float xyz_scale = 1/1.0888f; 

const char possible_bayer_patterns[] = "0123";

typedef struct {
	int bayer_pattern;
	int luma_only;
	int force_rb_to_zero;
	int input_data_width;
	int output_data_width;
	int image_order_out;
	int plane_multiple;
	FIXED1_7 gradient_mul;
	uint16_t th1;
	uint16_t th2;
	FIXED8_8 slope;
	int16_t offset;
} DebayerParameters;

class DebayerFilt : public SippBaseFilt {

public:
	DebayerFilt(SippIrq *pObj = 0,
		int id = 0,
		int ahdk = 0,
		int bilk = 0,
		std::string name = "Debayer filter");
	~DebayerFilt() {
	}

	void SetBayerPattern(int, int reg = 0);
	void SetLumaOnly(int, int reg = 0);
	void SetForceRbToZero(int, int reg = 0);
	void SetInputDataWidth(int, int reg = 0);
	void SetOutputDataWidth(int, int reg = 0);
	void SetImageOrderOut(int, int reg = 0);
	void SetPlaneMultiple(int, int reg = 0);
	void SetGradientMul(int, int reg = 0);
	void SetThreshold1(int, int reg = 0);
	void SetThreshold2(int, int reg = 0);
	void SetSlope(int, int reg = 0);
	void SetOffset(int, int reg = 0);

	void DeterminePlanes();
	void PromoteTo16Bits(uint8_t  * , uint16_t *, int);
	void PromoteTo16Bits(uint16_t * , uint16_t *, int);
	void GetMaximumGradient(uint16_t **, uint32_t *, int, int);
	void BilinearDebayer(uint16_t **, uint16_t *, uint32_t *, uint16_t *[3]);

	int GetBayerPattern(int reg = 0)    { return bayer_pattern; };
	int GetLumaOnly(int reg = 0)        { return luma_only; };
	int GetForceRbToZero(int reg = 0)   { return force_rb_to_zero; };
	int GetInputDataWidth(int reg = 0)  { return input_data_width; };
	int GetOutputDataWidth(int reg = 0) { return output_data_width; };
	int GetImageOrderOut(int reg = 0)   { return image_order_out; };
	int GetPlaneMultiple(int reg = 0)   { return plane_multiple; };
	int GetGradientMul(int reg = 0)     { return gradient_mul.full; };
	int GetThreshold1(int reg = 0)      { return th1; };
	int GetThreshold2(int reg = 0)      { return th2; };
	int GetSlope(int reg = 0)           { return slope.full; };
	int GetOffset(int reg = 0)          { return offset; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	DebayerParameters dbyr_params[2];
	int ahd_kernel_size;
	int bil_kernel_size;
	int bayer_pattern;
	int luma_only;
	int force_rb_to_zero;
	int input_data_width;
	int output_data_width;
	int image_order_out;
	int plane_multiple;
	FIXED1_7 gradient_mul;
	uint16_t th1;
	uint16_t th2;
	FIXED8_8 slope;
	int16_t offset;
	uint16_t xyz_cam[3][3];
	int planes[3];
	int output_shift;
	int full_set;
	int remainder;
	int crt_output_planes;
	int crt_output_sp;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the Debayer filter
	void Run(void **, void **);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_DBYR_H__
