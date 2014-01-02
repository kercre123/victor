// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Programmable convolution filter
//
//
// -----------------------------------------------------------------------------

#include "sippHarrisCorners.h"
#include "sippDebug.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#else
#include <cmath>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <climits>
#include <cfloat>
#include <math.h>

// Need to set rounding mode to match HW for Harris score
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif

HarrisCornersFilt::HarrisCornersFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
}

void HarrisCornersFilt::SetKernelSize(int val, int reg) {
	if(!strchr(harris_kernel_sizes, (char)(val + '0'))) {
		std::cerr << "Invalid kernel size: possible sizes are 5, 7 or 9." << std::endl;
		abort();
	}
	else {
		harr_params[reg].kernel_size = val;
		base_params[reg].max_pad = 0;
	}
}

void HarrisCornersFilt::SetK(int val, int reg) {
	harr_params[reg].k = *(float *)&val;
}

int HarrisCornersFilt::DxConvolution(uint8_t *kernel, int kernel_size, int ln, int pix) {
	int k_i, k_j, sk_i, sk_j;
	int ret = 0;

	for(k_i = ln, sk_i = 0; k_i < ln + RADIUS; k_i++, sk_i++)
		for(k_j = pix, sk_j = 0; k_j < pix + RADIUS; k_j++, sk_j++)
			ret += kernel[k_i*kernel_size + k_j] * sub_kernel[sk_i*RADIUS + sk_j];

	return ret;
}

int HarrisCornersFilt::DyConvolution(uint8_t *kernel, int kernel_size, int ln, int pix) {
	int k_i, k_j, sk_i, sk_j;
	int ret = 0;

	for(k_i = ln, sk_i = 0; k_i < ln + RADIUS; k_i++, sk_i++)
		for(k_j = pix, sk_j = 0; k_j < pix + RADIUS; k_j++, sk_j++)
			ret += kernel[k_i*kernel_size + k_j] * sub_kernel[sk_j*RADIUS + sk_i];

	return ret;
}

float HarrisCornersFilt::Sum(float *arr, int size) {
	float s = 0.0f;

	for(int i = 0; i < size; i++)
		s += arr[i];
	
	return s;
}

void HarrisCornersFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	kernel_size = harr_params[reg].kernel_size;
	k           = harr_params[reg].k;
}

void HarrisCornersFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr    = 0;
	uint8_t **      in_ptr    = 0;
	fp16    *      out_ptr_16 = 0;
	float   *      out_ptr_32 = 0;

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

		if(out.GetFormat() == SIPP_FORMAT_16BIT) {
			setOutputLine(&out_ptr_16, pl);
			Run((void **)in_ptr, (void *)out_ptr_16);
		} else {
			setOutputLine(&out_ptr_32, pl);
			Run((void **)in_ptr, (void *)out_ptr_32);
		}
	}
	// Update buffer fill levels and input frame line index 
	UpdateBuffersSyncNoPad(kernel_size);
	IncLineIdxNoPad(kernel_size);

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	delete[] split_in_ptr;
	delete[]       in_ptr;
}

void HarrisCornersFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr    = 0;
	uint8_t **      in_ptr    = 0;
	fp16    *      out_ptr_16 = 0;
	float   *      out_ptr_32 = 0;

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
	while (CanRunNoPad(kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			getKernelLines(split_in_ptr, kernel_size, pl);
			PackInputLines(split_in_ptr, kernel_size, in_ptr);

			if(out.GetFormat() == SIPP_FORMAT_16BIT) {
				setOutputLine(&out_ptr_16, pl);
				Run((void **)in_ptr, (void *)out_ptr_16);
			} else {
				setOutputLine(&out_ptr_32, pl);
				Run((void **)in_ptr, (void *)out_ptr_32);
			}
		}
		// Update buffer fill levels and input frame line index 
		UpdateBuffersNoPad(kernel_size);
		IncLineIdxNoPad(kernel_size);

		// At end of frame	
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

void *HarrisCornersFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t **      src    = 0;
	fp16    *       dst_16 = 0;
	fp16    *packed_dst_16 = 0;
	float   *       dst_32 = 0;
	float   *packed_dst_32 = 0;

	uint8_t *kernel;
	int *dx, *dy;
	float *xx,    *xy,    *yy;
	float *xx_sr, *xy_sr, *yy_sr;

	// Allocating array of pointers to input lines
	src = new uint8_t *[kernel_size];

	// Setting input pointers
	for (int i = 0; i < kernel_size; i++)
		src[i] = (uint8_t *)in_ptr[i];

	// 2D kernel
	kernel = new uint8_t[kernel_size*kernel_size];

	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_16BIT) {
		       dst_16 = (fp16  *)out_ptr;
		packed_dst_16 = new fp16 [width - kernel_size + 1];
	} else {
		       dst_32 = (float *)out_ptr;
		packed_dst_32 = new float[width - kernel_size + 1];
	}
	
	dx    = new int  [kernel_size];
	dy    = new int  [kernel_size];
	xx    = new float[kernel_size];
	xy    = new float[kernel_size];
	yy    = new float[kernel_size];
	xx_sr = new float[kernel_size];
	xy_sr = new float[kernel_size];
	yy_sr = new float[kernel_size];

	int x = 0;
	float final_xx, final_xy, final_yy;
	float final_xx_yy, final_xy_xy;
	float det, trace;

	int sign, exp16, exp32, sig16, sig32;
	uint16_t packed;
	float out_val_32;
	fp16 out_val_16;

	while (x < width - (kernel_size - 1)) {
		// Build the 2D kernel
		buildKernel(src, kernel, kernel_size, x);

		for(int pix = 0; pix < kernel_size - 2; pix++) {
			for(int ln = 0; ln < kernel_size - 2; ln++) {
				// Reset values
				xx[ln] = xy[ln] = yy[ln] = 0.0f;

				// Compute dx and dy
				dx[ln] = DxConvolution(kernel, kernel_size, ln, pix);
				dy[ln] = DyConvolution(kernel, kernel_size, ln, pix);

				// Accumulate xx, xy and yy
				xx[ln] += dx[ln] * dx[ln];
				xy[ln] += dx[ln] * dy[ln];
				yy[ln] += dy[ln] * dy[ln];
			}

			// Shift values into shift registers
			xx_sr[pix] = Sum(xx, kernel_size - 2);
			xy_sr[pix] = Sum(xy, kernel_size - 2);
			yy_sr[pix] = Sum(yy, kernel_size - 2);
		}

		// Set rounding mode towards zero
		unsigned int rnd;
#ifndef WIN32
		rnd = fegetround();
		fesetround(FE_TOWARDZERO);
#else
		_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif
		// Compute final xx, xy and yy
		final_xx = Sum(xx_sr, kernel_size - 2);
		final_xy = Sum(xy_sr, kernel_size - 2);
		final_yy = Sum(yy_sr, kernel_size - 2);

		// Compute the determinant and the trace
		final_xx_yy = final_xx * final_yy;
		final_xy_xy = final_xy * final_xy;
		det =  final_xx_yy - final_xy_xy;
		trace = final_xx + final_yy;

		// Determine and store the Harris score
		float trace2, ktrace2;
		trace2  = trace * trace;
		ktrace2 = k * trace2;
		out_val_32 = det - ktrace2;
		if(out.GetFormat() == SIPP_FORMAT_16BIT) {
			// Bitwise manipulation (fp32 to scaled fp16)
			exp32 = EXTRACT_F32_EXP(*(int *)&out_val_32);
			exp16 = ((exp32 - 127) - FP16_TO_FP32_SCALE) + 15;
			sign = out_val_32 < 0.0f;
			if(exp16 < 0x1f) {
				sig32 = EXTRACT_F32_FRAC(*(int *)&out_val_32);
				sig16 = sig32 >> 13;
				packed = (uint16_t)PACK_F16(sign, exp16, sig16);
				out_val_16.setPackedValue(packed);
			} else {
				out_val_16 = (sign) ? -FP16_MAX : FP16_MAX;
			}
			packed_dst_16[x] = out_val_16;
		} else {
			packed_dst_32[x] = ClampWR(out_val_32, -FLT_MAX, FLT_MAX);
		}
		x++;

		// Restore the rounding mode
#ifndef WIN32
		fesetround(rnd);
#else
		_controlfp_s(0, rnd, _MCW_RC);
#endif
	}

	if(out.GetFormat() == SIPP_FORMAT_16BIT) {
		SplitOutputLine(packed_dst_16, width - kernel_size + 1, dst_16);
		delete[] packed_dst_16;
	} else {
		SplitOutputLine(packed_dst_32, width - kernel_size + 1, dst_32);
		delete[] packed_dst_32;
	}

	delete[] dx;    delete[] dy;
	delete[] xx;    delete[] xy;    delete[] yy;
	delete[] xx_sr; delete[] xy_sr; delete[] yy_sr;
	delete[] kernel;
	delete[] src;

	return out_ptr;
}
