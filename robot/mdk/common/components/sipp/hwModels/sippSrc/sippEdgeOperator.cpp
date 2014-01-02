// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Edge operator filter
//
//
// -----------------------------------------------------------------------------

#include "sippEdgeOperator.h"

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
#include <math.h>


EdgeOperatorFilt::EdgeOperatorFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);
}

void EdgeOperatorFilt::SetInputMode(int val, int reg) {
	edge_params[reg].input_mode = val;
	if(edge_params[reg].input_mode == PRE_FP16_GRAD ||
	   edge_params[reg].input_mode == PRE_U8_GRAD) {
		kernel_size = 1;
		base_params[reg].max_pad = kernel_size >> 1;
	}
}

void EdgeOperatorFilt::SetOutputMode(int val, int reg) {
	edge_params[reg].output_mode = val;

	int clip;
	if(edge_params[reg].output_mode == SCALED_MAGN_8BIT  || 
	   edge_params[reg].output_mode == ORIENT_8BIT       || 
	   edge_params[reg].output_mode == MAGN_ORIENT_16BIT)
		clip = (1 <<  8) - 1;
	else
		clip = (1 << 16) - 1;
	magn_top_clip = fp16((float)clip);
}

void EdgeOperatorFilt::SetThetaMode(int val, int reg) {
	edge_params[reg].theta_mode = val;
}

void EdgeOperatorFilt::SetMagnScf(int val, int reg) {
	edge_params[reg].magn_scale_factor.setPackedValue((uint16_t)val);
}

void EdgeOperatorFilt::SetXCoeff_a(int val, int reg) {
	edge_params[reg].h_coeffs[0] = val;
}

void EdgeOperatorFilt::SetXCoeff_b(int val, int reg) {
	edge_params[reg].h_coeffs[1] = val;
}

void EdgeOperatorFilt::SetXCoeff_c(int val, int reg) {
	edge_params[reg].h_coeffs[2] = val;
}

void EdgeOperatorFilt::SetXCoeff_d(int val, int reg) {
	edge_params[reg].h_coeffs[3] = val;
}

void EdgeOperatorFilt::SetXCoeff_e(int val, int reg) {
	edge_params[reg].h_coeffs[4] = val;
}

void EdgeOperatorFilt::SetXCoeff_f(int val, int reg) {
	edge_params[reg].h_coeffs[5] = val;
}

void EdgeOperatorFilt::SetYCoeff_a(int val, int reg) {
	edge_params[reg].v_coeffs[0] = val;
}

void EdgeOperatorFilt::SetYCoeff_b(int val, int reg) {
	edge_params[reg].v_coeffs[1] = val;
}

void EdgeOperatorFilt::SetYCoeff_c(int val, int reg) {
	edge_params[reg].v_coeffs[2] = val;
}

void EdgeOperatorFilt::SetYCoeff_d(int val, int reg) {
	edge_params[reg].v_coeffs[3] = val;
}

void EdgeOperatorFilt::SetYCoeff_e(int val, int reg) {
	edge_params[reg].v_coeffs[4] = val;
}

void EdgeOperatorFilt::SetYCoeff_f(int val, int reg) {
	edge_params[reg].v_coeffs[5] = val;
}

// Determine absolute angle
int EdgeOperatorFilt::DetermineAbsoluteAngle(int primary_theta, fp16 gx, fp16 gy) {
	bool agx_smaller;
	int sign_gx, sign_gy, exp_gx, exp_gy, frac_gx, frac_gy;
	uint16_t pgx, pgy;

	// Following hackery ensures correct octant selection for subnormals
	sign_gx = EXTRACT_F16_SIGN(gx.getPackedValue());
	sign_gy = EXTRACT_F16_SIGN(gy.getPackedValue());
	exp_gx  = EXTRACT_F16_EXP (gx.getPackedValue());
	exp_gy  = EXTRACT_F16_EXP (gy.getPackedValue());
	frac_gx = EXTRACT_F16_FRAC(gx.getPackedValue());
	frac_gy = EXTRACT_F16_FRAC(gy.getPackedValue());
	if(((exp_gx == 0 && frac_gx != 0) || (exp_gx == 0 && frac_gx == 0)) && 
	    (exp_gy == 0 && frac_gy != 0)) {
		agx_smaller = frac_gx < frac_gy;
	} else {
		fp16 agx = fabs(gx);
		fp16 agy = fabs(gy);
		agx_smaller = agx < agy;
	}

	// Reflect in the X axis
	pgx = gx.getPackedValue();
	pgy = gy.getPackedValue();
	if(theta_mode != NORMAL_THETA)
		if(pgy & FP16_SIGN)
			pgy &= ~FP16_SIGN;

	// Reflect in the Y axis
	if(theta_mode == XY_AXIS_REFL)
		if(pgx & FP16_SIGN)
			pgx &= ~FP16_SIGN;

	// Select corresponding octant for the orientation angle
	if(agx_smaller) {
		if(pgx & FP16_SIGN) {
			if(pgy & FP16_SIGN)
				return 107 - primary_theta;
			else
				return 36  + primary_theta;
		} else {
			if(pgy & FP16_SIGN)
				return 108 + primary_theta;
			else
				return 35  - primary_theta;
		}
	} else {
		if(pgx & FP16_SIGN) {
			if(pgy & FP16_SIGN)
				return 72  + primary_theta;
			else
				return 71  - primary_theta;
		} else {
			if(pgy & FP16_SIGN)
				return 143 - primary_theta;
			else
				return       primary_theta;
		}
	}
}

int EdgeOperatorFilt::DetermineSQRLUTPosition(float a) {
	return (int)(a/STEP_SQR);
}

int EdgeOperatorFilt::DetermineATANLUTPosition(fp16 tan_theta) {
	int i = 0;

	if(tan_theta == 0.f) {
		return 0;
	} else {
		while(tan_theta > atan_approx_LUT[i])
			i++;
		return i - 1;
	}
}

int EdgeOperatorFilt::DetermineCoeffValue(int coeff) {
	if(!(coeff & 0x10)) {
		return coeff;
	} else {
		coeff |= COEFF_MASK;
		return coeff;
	}
}

void EdgeOperatorFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	input_mode        = edge_params[reg].input_mode;
	output_mode       = edge_params[reg].output_mode;
	theta_mode        = edge_params[reg].theta_mode;
	magn_scale_factor = edge_params[reg].magn_scale_factor;
	for(int c = 0; c < 6; c++) {
		h_coeffs[c] = edge_params[reg].h_coeffs[c];
		v_coeffs[c] = edge_params[reg].v_coeffs[c];
	}
}

void EdgeOperatorFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint32_t **split_in_ptr_32 = 0;
	uint32_t **      in_ptr_32 = 0;
	uint8_t  *      out_ptr_8  = 0;
	uint16_t *      out_ptr_16 = 0;
	uint32_t *      out_ptr_32 = 0;

	void **in_ptr;
	void *out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(input_mode == NORMAL_MODE) {
		split_in_ptr_8  = new uint8_t  *[kernel_size];
		      in_ptr_8  = new uint8_t  *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else if(input_mode == PRE_U8_GRAD) {
		split_in_ptr_16 = new uint16_t *[kernel_size];
		      in_ptr_16 = new uint16_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	} else {
		split_in_ptr_32 = new uint32_t *[kernel_size];
		      in_ptr_32 = new uint32_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_32[ln] = new uint32_t[width];
		in_ptr = (void **)in_ptr_32;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Filter a line from each plane
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Prepare filter buffer line pointers then run
		if(input_mode == NORMAL_MODE) {
			getKernelLines(split_in_ptr_8 , kernel_size, pl);
			PackInputLines(split_in_ptr_8 , kernel_size, in_ptr_8 );
		} else if(input_mode == PRE_U8_GRAD) {
			getKernelLines(split_in_ptr_16, kernel_size, pl);
			PackInputLines(split_in_ptr_16, kernel_size, in_ptr_16);
		} else {
			getKernelLines(split_in_ptr_32, kernel_size, pl);
			PackInputLines(split_in_ptr_32, kernel_size, in_ptr_32);
		}
		
		if(output_mode == SCALED_MAGN_8BIT || output_mode == ORIENT_8BIT) {
			setOutputLine(&out_ptr_8 , pl);
			out_ptr = (void *)out_ptr_8;
		} else if(output_mode == SCALED_GRADIENTS_32BIT) {
			setOutputLine(&out_ptr_32, pl);
			out_ptr = (void *)out_ptr_32;
		} else {
			setOutputLine(&out_ptr_16, pl);
			out_ptr = (void *)out_ptr_16;
		}
		Run(in_ptr, out_ptr);
	}
	// Update buffer fill levels and input frame line index 
	UpdateBuffersSync();
	IncLineIdx();

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	if(input_mode == NORMAL_MODE) {
		delete[] split_in_ptr_8 ;
		delete[]       in_ptr_8 ;
	} else if(input_mode == PRE_U8_GRAD) {
		delete[] split_in_ptr_16;
		delete[]       in_ptr_16;
	} else {
		delete[] split_in_ptr_32;
		delete[]       in_ptr_32;
	}
}

void EdgeOperatorFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint32_t **split_in_ptr_32 = 0;
	uint32_t **      in_ptr_32 = 0;
	uint8_t  *      out_ptr_8  = 0;
	uint16_t *      out_ptr_16 = 0;
	uint32_t *      out_ptr_32 = 0;

	void **in_ptr;
	void *out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(input_mode == NORMAL_MODE) {
		split_in_ptr_8  = new uint8_t  *[kernel_size];
		      in_ptr_8  = new uint8_t  *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else if(input_mode == PRE_U8_GRAD) {
		split_in_ptr_16 = new uint16_t *[kernel_size];
		      in_ptr_16 = new uint16_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	} else {
		split_in_ptr_32 = new uint32_t *[kernel_size];
		      in_ptr_32 = new uint32_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_32[ln] = new uint32_t[width];
		in_ptr = (void **)in_ptr_32;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			if(input_mode == NORMAL_MODE) {
				getKernelLines(split_in_ptr_8 , kernel_size, pl);
				PackInputLines(split_in_ptr_8 , kernel_size, in_ptr_8 );
			} else if(input_mode == PRE_U8_GRAD) {
				getKernelLines(split_in_ptr_16, kernel_size, pl);
				PackInputLines(split_in_ptr_16, kernel_size, in_ptr_16);
			} else {
				getKernelLines(split_in_ptr_32, kernel_size, pl);
				PackInputLines(split_in_ptr_32, kernel_size, in_ptr_32);
			}
			
			if(output_mode == SCALED_MAGN_8BIT || output_mode == ORIENT_8BIT) {
				setOutputLine(&out_ptr_8 , pl);
				out_ptr = (void *)out_ptr_8;
			} else if(output_mode == SCALED_GRADIENTS_32BIT) {
				setOutputLine(&out_ptr_32, pl);
				out_ptr = (void *)out_ptr_32;
			} else {
				setOutputLine(&out_ptr_16, pl);
				out_ptr = (void *)out_ptr_16;
			}
			Run(in_ptr, out_ptr);
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

	if(input_mode == NORMAL_MODE) {
		delete[] split_in_ptr_8 ;
		delete[]       in_ptr_8 ;
	} else if(input_mode == PRE_U8_GRAD) {
		delete[] split_in_ptr_16;
		delete[]       in_ptr_16;
	} else {
		delete[] split_in_ptr_32;
		delete[]       in_ptr_32;
	}
}

void *EdgeOperatorFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t  **      src_8  = 0;
	uint16_t *       src_16 = 0;
	uint32_t *       src_32 = 0;
	uint8_t  *       dst_8  = 0;
	uint8_t  *packed_dst_8  = 0;
	uint16_t *       dst_16 = 0;
	uint16_t *packed_dst_16 = 0;
	uint32_t *       dst_32 = 0;
	uint32_t *packed_dst_32 = 0;

	uint8_t *kernel;

	// Allocating array of pointers to input lines
	// and setting input pointers
	if(input_mode == NORMAL_MODE) {
		src_8 = new uint8_t *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			src_8[i] = (uint8_t *)in_ptr[i];
	} else if(input_mode == PRE_U8_GRAD) {
		src_16 = (uint16_t *)in_ptr[0];
	} else {
		src_32 = (uint32_t *)in_ptr[0];
	}

	// 2D kernel for convolution
	kernel = new uint8_t[kernel_size*kernel_size];

	// Setting output pointer and allocating temporary packed output buffer
	if(output_mode == SCALED_MAGN_8BIT || output_mode == ORIENT_8BIT) {
		       dst_8  = (uint8_t  *)out_ptr;
		packed_dst_8  = new uint8_t [width];
	} else if(output_mode == SCALED_GRADIENTS_32BIT) {
		       dst_32 = (uint32_t *)out_ptr;
		packed_dst_32 = new uint32_t[width];
	} else {
		       dst_16 = (uint16_t *)out_ptr;
		packed_dst_16 = new uint16_t[width];
	}

	int x = 0;

	int k_i, hc_i, vc_i;
	int gx, gy;
	fp16 gx_fp16, gy_fp16, agx_fp16, agy_fp16;
	fp16 grad_ratio;
	int lut_pos;
	fp16 magn;
	int primary_theta, absolute_theta;

	while (x < width) {
		// Compute gradients if on normal input mode
		if(input_mode == NORMAL_MODE) {
			// Build the 2D kernel
			buildKernel(src_8, kernel, kernel_size, x);

			// Reset gradients
			gx = gy = 0;

			for(k_i = 0, hc_i = 0, vc_i = 0; k_i < kernel_size * kernel_size; k_i++) {
				// Rule out second column (only zeroes)
				if(k_i % kernel_size != 1)
					gx += kernel[k_i] * DetermineCoeffValue(h_coeffs[hc_i++]);

				// Rule out second row (only zeroes)
				if(k_i < kernel_size || k_i >= (kernel_size << 1))
					gy += kernel[k_i] * DetermineCoeffValue(v_coeffs[vc_i++]);
			}

			// Determine gradients' fp16 equivalents
			gx_fp16 = (fp16)((float)gx);
			gy_fp16 = (fp16)((float)gy);
		} else if(input_mode == PRE_U8_GRAD) {
			// Create the u8 gradients
			gx = src_16[x] & EIGHTBITS;
			gy = src_16[x] >> 8;

			// Determine gradients' fp16 equivalents
			gx_fp16 = (fp16)((float)gx);
			gy_fp16 = (fp16)((float)gy);
		} else {
			// Create the fp16 gradients
			gx_fp16.setPackedValue(src_32[x] & SIXTEENBITS);
			gy_fp16.setPackedValue(src_32[x] >> 16);

			// Determine gradients' int equivalents
			gx = (int)(gx_fp16);
			gy = (int)(gy_fp16);
		}

		// Output gradients if requested (either packed within 16 or 32 bits)
		if(output_mode == SCALED_GRADIENTS_16BIT) {
			// Scale gradients 
			gx_fp16 = gx_fp16 * magn_scale_factor;
			gy_fp16 = gy_fp16 * magn_scale_factor;

			// Clamp gradients' int equivalents
			gx = ClampWR((int)gx_fp16, -128, 127);
			gy = ClampWR((int)gy_fp16, -128, 127);
			packed_dst_16[x] = (gy & EIGHTBITS) << 8 | gx & EIGHTBITS;
			x++;
			continue;
		}

		if(output_mode == SCALED_GRADIENTS_32BIT) {
			// Scale gradients 
			gx_fp16 = gx_fp16 * magn_scale_factor;
			gy_fp16 = gy_fp16 * magn_scale_factor;
			packed_dst_32[x] = (gy_fp16.getPackedValue() << 16) | gx_fp16.getPackedValue();
			x++;
			continue;
		}

		// Determine the LUT position necessary for interpolation
		agx_fp16 = fabs(gx_fp16);
		agy_fp16 = fabs(gy_fp16);
		if(agx_fp16 == 0.f && agx_fp16 == agy_fp16)
			grad_ratio = fp16(0.f);
		else
			grad_ratio = (agx_fp16 > agy_fp16) ? agy_fp16/agx_fp16 : agx_fp16/agy_fp16;
		lut_pos = DetermineSQRLUTPosition(grad_ratio);

		// Approximate the magnitude
		magn = (fp16)Interpolate(grad_ratio, lut_pos*STEP_SQR, (lut_pos + 1)*STEP_SQR, sqr_approx_LUT[lut_pos], sqr_approx_LUT[lut_pos + 1]);
		magn *= (agx_fp16 > agy_fp16) ? agx_fp16 * magn_scale_factor : agy_fp16 * magn_scale_factor;

		// Approximate the primary angle
		primary_theta = DetermineATANLUTPosition(grad_ratio);

		// Compute the absolute angle
		absolute_theta = DetermineAbsoluteAngle(primary_theta, gx_fp16, gy_fp16);

		// Store the result based on the output mode
		if(output_mode == SCALED_MAGN_8BIT)
			packed_dst_8 [x] = (uint8_t)CLIPF(magn, magn_top_clip);
		if(output_mode == SCALED_MAGN_16BIT)
			packed_dst_16[x] = (uint16_t)CLIPF(magn, magn_top_clip);
		if(output_mode == MAGN_ORIENT_16BIT)
			packed_dst_16[x] = (((uint8_t)CLIPF(magn, magn_top_clip) & EIGHTBITS) << 8) | (uint8_t)absolute_theta & EIGHTBITS;
		if(output_mode == ORIENT_8BIT)
			packed_dst_8 [x] = (uint8_t)absolute_theta;
		x++;
	}

	if(output_mode == SCALED_MAGN_8BIT || output_mode == ORIENT_8BIT) {
		SplitOutputLine(packed_dst_8 , width, dst_8 );
		delete[] packed_dst_8 ;
	} else if(output_mode == SCALED_GRADIENTS_32BIT) {
		SplitOutputLine(packed_dst_32, width, dst_32);
		delete[] packed_dst_32;
	} else {
		SplitOutputLine(packed_dst_16, width, dst_16);
		delete[] packed_dst_16;
	}

	delete[] kernel;
	if(input_mode == NORMAL_MODE)
		delete[] src_8;

	return out_ptr;
}

