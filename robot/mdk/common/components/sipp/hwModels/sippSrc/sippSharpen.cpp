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

#include "sippSharpen.h"

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
#include <float.h>

// Need to set rounding mode to match HW
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif

SharpenFilt::SharpenFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);

		for(int i = 0 ; i < 4 ; i++)
			blur_coeffs[i] = 0;
}

void SharpenFilt::SetKernelSize(int val, int reg) {
	if(!strchr(sharpen_kernel_sizes, (char)(val + '0'))) {
		std::cerr << "Invalid kernel size: possible sizes are 3, 5 or 7." << std::endl;
		abort();
	}
	else {
		usm_params[reg].kernel_size = val;
		base_params[reg].max_pad = val >> 1;
		coeff_offset = (val == 7) ? 0 : (val == 5) ? 1 : 2;
	}
}

void SharpenFilt::SetOutputClamp(int val, int reg) {
	usm_params[reg].output_clamp = (out.GetFormat() == SIPP_FORMAT_8BIT) ? 1 : val;
}

void SharpenFilt::SetMode(int val, int reg) {
	if(val != BLUR && val != SHARPEN) {
		std::cerr << "Invalid mode: possible modes are BLUR(=1) or SHARPEN(=0)." << std::endl;
		abort();
	}
	else
		usm_params[reg].mode = val;
}

void SharpenFilt::SetOutputDeltas(int val, int reg) {
	if(val)
		if(out.GetFormat() != SIPP_FORMAT_16BIT) {
			std::cerr << "Invalid configuration. Deltas can be output only in FP16." << std::endl;
			abort();
		}
	usm_params[reg].output_deltas = val;
}

void SharpenFilt::SetThreshold(int val, int reg) {
	usm_params[reg].threshold.setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetPosStrength(int val, int reg) {
	usm_params[reg].strength_pos.setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetNegStrength(int val, int reg) {
	usm_params[reg].strength_neg.setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetClippingAlpha(int val, int reg) {
	usm_params[reg].clipping_alpha.setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetUndershoot(int val, int reg) {
	fp16 tmp;
	tmp.setPackedValue((unsigned short)val);
	float f32_tmp = tmp.getUnpackedValue();
	if(f32_tmp < 0.0f || f32_tmp > 1.0f) {
		std::cerr << "Invalid undershoot: must be in range [0.0 - 1.0]." << std::endl;
		abort();
	}
	usm_params[reg].undershoot = fp16(tmp);
}

void SharpenFilt::SetOvershoot(int val, int reg) {
	fp16 tmp;
	tmp.setPackedValue((unsigned short)val);
	float f32_tmp = tmp.getUnpackedValue();
	if(f32_tmp < 1.0f || f32_tmp > 2.0f) {
		std::cerr << "Invalid overshoot: must be in range [1.0 - 2.0]." << std::endl;
		abort();
	}
	usm_params[reg].overshoot = fp16(tmp);
}

void SharpenFilt::SetRngstop0(int val, int reg) {
	usm_params[reg].range_stops[0].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetRngstop1(int val, int reg) {
	usm_params[reg].range_stops[1].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetRngstop2(int val, int reg) {
	usm_params[reg].range_stops[2].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetRngstop3(int val, int reg) {
	usm_params[reg].range_stops[3].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetCoeff0(int val, int reg) {
	usm_params[reg].blur_coeffs[0].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetCoeff1(int val, int reg) {
	usm_params[reg].blur_coeffs[1].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetCoeff2(int val, int reg) {
	usm_params[reg].blur_coeffs[2].setPackedValue((unsigned short)(val & 0xffff));
}

void SharpenFilt::SetCoeff3(int val, int reg) {
	usm_params[reg].blur_coeffs[3].setPackedValue((unsigned short)(val & 0xffff));
	// Auto complete the coefficient array after setting 
	// the last one by mirroring the existing ones
	AutoCompleteArray();
}

void SharpenFilt::AutoCompleteArray() {	
	for(int i = 0, first_to_fill = 4, first_filler = 2; i < kernel_size/2; i++, first_to_fill++, first_filler--)
		usm_params[reg].blur_coeffs[first_to_fill] = usm_params[reg].blur_coeffs[first_filler];
}

float SharpenFilt::GetSmoothPixel(fp16 *kernel, int kernel_size) {
	int pix;
	float smooth = 0.0f;

	// Smoothen pixel from a 3x3 neighbourhood base on kernel's size
	switch(kernel_size) {
	case 3 :
		pix = 0;
		smooth += kernel[pix]                   * smooth_kernel[0]/8.0f +
		          kernel[pix+1]                 * smooth_kernel[1]/8.0f +
				  kernel[pix+2]                 * smooth_kernel[2]/8.0f +
				  kernel[pix+kernel_size]       * smooth_kernel[3]/8.0f +
				  kernel[pix+kernel_size+1]     * smooth_kernel[4]/8.0f +
				  kernel[pix+kernel_size+2]     * smooth_kernel[5]/8.0f +
				  kernel[pix+(kernel_size<<1)]  * smooth_kernel[6]/8.0f +
				  kernel[pix+(kernel_size<<1)+1]* smooth_kernel[7]/8.0f +
				  kernel[pix+(kernel_size<<1)+2]* smooth_kernel[8]/8.0f;
		break;
	case 5 :
		pix = 6;
		smooth += kernel[pix]                   * smooth_kernel[0]/8.0f +
		          kernel[pix+1]                 * smooth_kernel[1]/8.0f +
				  kernel[pix+2]                 * smooth_kernel[2]/8.0f +
				  kernel[pix+kernel_size]       * smooth_kernel[3]/8.0f +
				  kernel[pix+kernel_size+1]     * smooth_kernel[4]/8.0f +
				  kernel[pix+kernel_size+2]     * smooth_kernel[5]/8.0f +
				  kernel[pix+(kernel_size<<1)]  * smooth_kernel[6]/8.0f +
				  kernel[pix+(kernel_size<<1)+1]* smooth_kernel[7]/8.0f +
				  kernel[pix+(kernel_size<<1)+2]* smooth_kernel[8]/8.0f;
		break;
	case 7 :
		pix = 16;
		smooth += kernel[pix]                   * smooth_kernel[0]/8.0f +
			      kernel[pix+1]                 * smooth_kernel[1]/8.0f +
				  kernel[pix+2]                 * smooth_kernel[2]/8.0f +
				  kernel[pix+kernel_size]       * smooth_kernel[3]/8.0f +
				  kernel[pix+kernel_size+1]     * smooth_kernel[4]/8.0f +
				  kernel[pix+kernel_size+2]     * smooth_kernel[5]/8.0f +
				  kernel[pix+(kernel_size<<1)]  * smooth_kernel[6]/8.0f +
				  kernel[pix+(kernel_size<<1)+1]* smooth_kernel[7]/8.0f +
			      kernel[pix+(kernel_size<<1)+2]* smooth_kernel[8]/8.0f;
		break;
	// No need for default branch since the kernel will have been set by this time
	}

	return smooth;
}

void SharpenFilt::BuildMMR(fp16 *kernel, float *mmr, int kernel_size) {
	int pix;

	switch(kernel_size) {
    case 3 :
		pix = 0;
		break;
	case 5 :
		pix = 6;
		break;
	case 7 :
		pix = 16;
		break;
	// Kernel size will have been set by this time, 
	// so this is solely for warning suppression purposes
	default :
		pix = -1;
		break;
	}
    mmr[0] = kernel[pix];                  
    mmr[1] = kernel[pix+1];                
    mmr[2] = kernel[pix+2];                
    mmr[3] = kernel[pix+kernel_size];      
    mmr[4] = kernel[pix+kernel_size+1];    
    mmr[5] = kernel[pix+kernel_size+2];    
    mmr[6] = kernel[pix+(kernel_size<<1)]; 
    mmr[7] = kernel[pix+(kernel_size<<1)+1];
    mmr[8] = kernel[pix+(kernel_size<<1)+2];
}

void SharpenFilt::GetMinMax(float *neigh, fp16 *min, fp16 *max) {
	*min = (float)INT_MAX;
	*max = (float)INT_MIN;
	for(int ni = 0; ni < 9; ni++) {
		if(*min > neigh[ni])
			*min = neigh[ni];
		if(*max < neigh[ni])
			*max = neigh[ni];
	}
}

void SharpenFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	kernel_size    = usm_params[reg].kernel_size;
	output_clamp   = usm_params[reg].output_clamp;
	mode           = usm_params[reg].mode;
	output_deltas  = usm_params[reg].output_deltas;
	threshold      = usm_params[reg].threshold;
	strength_pos   = usm_params[reg].strength_pos;
	strength_neg   = usm_params[reg].strength_neg;
	clipping_alpha = usm_params[reg].clipping_alpha;
	undershoot     = usm_params[reg].undershoot;
	overshoot      = usm_params[reg].overshoot;
	for(int rs = 0; rs < 4; rs++)
		range_stops[rs] = usm_params[reg].range_stops[rs];
	for(int bc = 0; bc < 7; bc++)
		blur_coeffs[bc] = usm_params[reg].blur_coeffs[bc];
}

void SharpenFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr_i = 0;
	uint8_t **      in_ptr_i = 0;
	fp16    **split_in_ptr_f = 0;
	fp16    **      in_ptr_f = 0;
	uint8_t *      out_ptr_i = 0;
	fp16    *      out_ptr_f = 0;

	void **in_ptr;
	void *out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_i = new uint8_t *[kernel_size];
		      in_ptr_i = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_i[ln] = new uint8_t[width];
		in_ptr = (void **)in_ptr_i;
	} else {
		split_in_ptr_f = new fp16 *[kernel_size];
		      in_ptr_f = new fp16 *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_f[ln] = new fp16[width];
		in_ptr = (void **)in_ptr_f;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Filter a line from each plane
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Prepare filter buffer line pointers then run
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_i, kernel_size, pl);
			PackInputLines(split_in_ptr_i, kernel_size, in_ptr_i);
		} else {
			getKernelLines(split_in_ptr_f, kernel_size, pl);
			PackInputLines(split_in_ptr_f, kernel_size, in_ptr_f);
		}

		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			setOutputLine(&out_ptr_i, pl);
			out_ptr = (void *)out_ptr_i;
		} else {
			setOutputLine(&out_ptr_f, pl);
			out_ptr = (void *)out_ptr_f;
		}

		Run(in_ptr, out_ptr);
	}
	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] split_in_ptr_i;
		delete[]       in_ptr_i;
	} else {
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
	}
}

void SharpenFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **split_in_ptr_i = 0;
	uint8_t **      in_ptr_i = 0;
	fp16    **split_in_ptr_f = 0;
	fp16    **      in_ptr_f = 0;
	uint8_t *      out_ptr_i = 0;
	fp16    *      out_ptr_f = 0;

	void **in_ptr;
	void *out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_i = new uint8_t *[kernel_size];
		      in_ptr_i = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_i[ln] = new uint8_t[width];
		in_ptr = (void **)in_ptr_i;
	} else {
		split_in_ptr_f = new fp16 *[kernel_size];
		      in_ptr_f = new fp16 *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr_f[ln] = new fp16[width];
		in_ptr = (void **)in_ptr_f;
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
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_in_ptr_i, kernel_size, pl);
				PackInputLines(split_in_ptr_i, kernel_size, in_ptr_i);
			} else {
				getKernelLines(split_in_ptr_f, kernel_size, pl);
				PackInputLines(split_in_ptr_f, kernel_size, in_ptr_f);
			}

			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				setOutputLine(&out_ptr_i, pl);
				out_ptr = (void *)out_ptr_i;
			} else {
				setOutputLine(&out_ptr_f, pl);
				out_ptr = (void *)out_ptr_f;
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

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] split_in_ptr_i;
		delete[]       in_ptr_i;
	} else {
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
	}
}

void *SharpenFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t **      src_i = 0;
	uint8_t * kernel_2D_i = 0;
	uint8_t *       dst_i = 0;
	uint8_t *packed_dst_i = 0;
	fp16 **         src_f = 0;
	fp16 *       kernel_f = 0;
	fp16 *    kernel_2D_f = 0;
	fp16 *          dst_f = 0;
	fp16 *   packed_dst_f = 0;

	// Allocating array of pointers to input lines, setting input pointers,
	// allocate the internal 1D kernel for the horizontal blur filter pass
	// and the internal 2D kernel for the smooth region
	kernel_f = new fp16[kernel_size];
	kernel_2D_f = new fp16[kernel_size*kernel_size];

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_i = new uint8_t *[kernel_size];
		kernel_2D_i = new uint8_t[kernel_size*kernel_size];
		for (int i = 0; i < kernel_size; i++)
			src_i[i] = (uint8_t *)in_ptr[i];
	} else {
		src_f = new fp16 *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			src_f[i] = (fp16 *)in_ptr[i];
	}

	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_i = (uint8_t *)out_ptr;
		packed_dst_i = new uint8_t[width];
	} else {
		       dst_f = (fp16    *)out_ptr;
		packed_dst_f = new fp16   [width];
	}

	// Output of vertical blur filter pass
	double *v_blurred = new double[width];
	fp16 *v_blurred2 = new fp16[width];

	double h_blurred;
	fp16 h_blurred2;
	fp16 mask;
	fp16 smooth;
	float mmr[9];
	fp16 min, max;
	fp16 temp_add;
	fp16 interp_value;
	fp16 crt_pixel;
	fp16 out_cl;
	fp16 out_val;
	fp16 out_blended;

	int ln, pix;
	int x = 0;

	// Set rounding mode towards zero to match RTL
	unsigned int rnd;
#ifndef WIN32
	rnd = fegetround();
	fesetround(FE_TOWARDZERO);
#else
	_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif
	
	// Vertical blur filter pass
	while (x < width) {
		v_blurred[x] = 0;
		for(ln = 0; ln < kernel_size/2; ln++) {
			// Exploit the coefficients' symmetry
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				temp_add  = U8F_TO_FP16(src_i[ln][x]);
				temp_add += U8F_TO_FP16(src_i[(kernel_size - ln - 1)][x]);
			} else {
				temp_add = src_f[ln][x] + src_f[(kernel_size - ln - 1)][x];
			}
			v_blurred[x] += fp16(temp_add * blur_coeffs[ln + coeff_offset]);
		}
			
		// Middle pixel
		if(in.GetFormat() == SIPP_FORMAT_8BIT)
			v_blurred[x] += fp16(U8F_TO_FP16(src_i[ln][x]) * blur_coeffs[ln + coeff_offset]);
		else
			v_blurred[x] += fp16(src_f[ln][x] * blur_coeffs[ln + coeff_offset]);
            
        v_blurred2[x] = fp16(v_blurred[x]);

		x++;
	}

	// Horizontal blur filter pass and unsharp mask
	x = 0;
	while (x < width) {
		// Horizontal padding of vertical blur filter output
		buildKernel(v_blurred2, kernel_f, kernel_size, x);
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			buildKernel(src_i, kernel_2D_i, kernel_size, x);
			convertKernel(kernel_2D_i, kernel_2D_f, kernel_size*kernel_size);
			crt_pixel = U8F_TO_FP16(src_i[kernel_size >> 1][x]);
		} else {
			buildKernel(src_f, kernel_2D_f, kernel_size, x);
			crt_pixel = src_f[kernel_size >> 1][x];
		}

		// Horizontal blur filter
		h_blurred = 0;
		for(pix = 0; pix < kernel_size/2; pix++) {
			// Exploit the coefficients' symmetry
			temp_add = kernel_f[pix] + kernel_f[(kernel_size - pix - 1)];
			h_blurred += fp16(temp_add * blur_coeffs[pix + coeff_offset]);
		}

		// Middle pixel
		h_blurred += fp16(kernel_f[pix] * blur_coeffs[pix + coeff_offset]);
		h_blurred2 = fp16(h_blurred);

		// Bypass the sharpening functionality
		if(mode == BLUR) {
			if(output_clamp)
				h_blurred2 = fp16(ClampWR(h_blurred2, 0.0f, 1.0f));
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_i[x] = FP16_TO_U8F(h_blurred2);
			else
				packed_dst_f[x] =             h_blurred2;
		}

		// Apply sharpening
		if(mode == SHARPEN) {
			// Subtract blurred version
			mask = fp16(crt_pixel - h_blurred2);

			// Determine the smooth pixel
			smooth = fp16(GetSmoothPixel(kernel_2D_f, kernel_size));

			// Suppress sharpening outside of the mid-tones
			if(smooth >= 0.0f && smooth <= range_stops[0]) {
				interp_value = 0;
			} else if(smooth > range_stops[0] && smooth <= range_stops[1]) {
			 	interp_value = Interpolate(smooth, range_stops[0], range_stops[1], 0.0f, 1.0f);
			} else if(smooth > range_stops[1] && smooth <= range_stops[2]) {
			 	interp_value = 1;
			} else if(smooth > range_stops[2] && smooth <= range_stops[3]) {
			 	interp_value = Interpolate(smooth, range_stops[3], range_stops[2], 0.0f, 1.0f);
			} else {
				interp_value = 0;
			}
			mask *= interp_value;

			// Cancel mask if less than a threshold. Otherwise, 
			// move it closer to 0 to get rid of the discontinuity
			if(threshold > 0.f) {
				mask = (mask >  threshold) * (mask - threshold) +
				       (mask < -threshold) * (mask + threshold);
			} else {
				mask = (mask <  threshold) * (mask - threshold) +
				       (mask > -threshold) * (mask + threshold);
			}

			// Either darken or brigthen the high-pass value
			if(mask > 0.f)
				mask *= strength_pos;
			else
				mask *= strength_neg;
			out_val = crt_pixel + mask;

			// Build 3x3 neighbourhood
			BuildMMR(kernel_2D_f, mmr, kernel_size);

			// Get the minimum and maximum values in a 3x3 neighbourhood
			GetMinMax(mmr, &min, &max);

			// Clamp to prevent excessive overshoot/undershoot and store the result
			out_cl = fp16(ClampWR(out_val, min*undershoot, max*overshoot));

			// Clamp blended value (if enabled)
			fp16 t1 = out_cl * clipping_alpha;
			fp16 t2 =  1.0f  - clipping_alpha;
			fp16 t3 = out_val * t2;
			out_blended = t1 + t3;
			if(output_clamp)
				out_blended = fp16(ClampWR(out_blended, 0.0f, 1.0f));

			// Output either the delta or the sharpened pixel
			if(output_deltas)
				packed_dst_f[x] = out_blended - crt_pixel;
			else if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_i[x] = FP16_TO_U8F(out_blended);
			else
				packed_dst_f[x] = out_blended;
		}
		x++;
	}

	// Restore the rounding mode
#ifndef WIN32
	fesetround(rnd);
#else
	_controlfp_s(0, rnd, _MCW_RC);
#endif

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		SplitOutputLine(packed_dst_i, width, dst_i);
		delete[] packed_dst_i;
	} else {
		SplitOutputLine(packed_dst_f, width, dst_f);
		delete[] packed_dst_f;
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] kernel_2D_i;
		delete[] src_i;
	} else {
		delete[] src_f;
	}
	delete[] kernel_f;
	delete[] kernel_2D_f;
	delete[] v_blurred;
	delete[] v_blurred2;

	return out_ptr;
}
