// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Programmable convolution filter
//
//
// -----------------------------------------------------------------------------

#include "sippConvolution.h"

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

// Need to set rounding mode to match HW for float accumulation
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif

ConvolutionFilt::ConvolutionFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetKernelLinesNo (k);
		SetId (id);

		for(int i = 0 ; i < 25 ; i++)
			coeffs[i] = 0;

		accum     = 0.0f;
		accum_cnt = 0;

		accum_result     = 0.0f;
		accum_cnt_result = 0;

		od_bit = 0;
}

void ConvolutionFilt::SetKernelSize(int val, int reg) {
	if(!strchr(conv_kernel_sizes, (char)(val + '0'))) {
		std::cerr << "Invalid kernel size: possible sizes are 3 or 5." << std::endl;
		abort();
	}
	else {
		conv_params[reg].kernel_size = val;
		base_params[reg].max_pad = val >> 1;
	}
}

void ConvolutionFilt::SetOutputClamp(int val, int reg) {
	conv_params[reg].output_clamp = (out.GetFormat() == SIPP_FORMAT_8BIT) ? 1 : val;
}

void ConvolutionFilt::SetAbsBit(int val, int reg) {
	conv_params[reg].abs_bit = val;
}

void ConvolutionFilt::SetSqBit(int val, int reg) {
	conv_params[reg].sq_bit = val;
}

void ConvolutionFilt::SetAccumBit(int val, int reg) {
	conv_params[reg].accum_bit = val;
}

void ConvolutionFilt::SetOdBit(int val, int reg) {
	conv_params[reg].od_bit = val;
}

void ConvolutionFilt::SetThreshold(int val, int reg) {
	conv_params[reg].thr.setPackedValue((uint16_t)val);
}

void ConvolutionFilt::SetCoeff00(int val, int reg) {
	conv_params[reg].coeffs[0].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff01(int val, int reg) {
	conv_params[reg].coeffs[1].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff02(int val, int reg) {
	conv_params[reg].coeffs[2].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff03(int val, int reg) {
	conv_params[reg].coeffs[3].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff04(int val, int reg) {
	conv_params[reg].coeffs[4].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff10(int val, int reg) {
	conv_params[reg].coeffs[5].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff11(int val, int reg) {
	conv_params[reg].coeffs[6].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff12(int val, int reg) {
	conv_params[reg].coeffs[7].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff13(int val, int reg) {
	conv_params[reg].coeffs[8].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff14(int val, int reg) {
	conv_params[reg].coeffs[9].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff20(int val, int reg) {
	conv_params[reg].coeffs[10].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff21(int val, int reg) {
	conv_params[reg].coeffs[11].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff22(int val, int reg) {
	conv_params[reg].coeffs[12].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff23(int val, int reg) {
	conv_params[reg].coeffs[13].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff24(int val, int reg) {
	conv_params[reg].coeffs[14].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff30(int val, int reg) {
	conv_params[reg].coeffs[15].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff31(int val, int reg) {
	conv_params[reg].coeffs[16].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff32(int val, int reg) {
	conv_params[reg].coeffs[17].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff33(int val, int reg) {
	conv_params[reg].coeffs[18].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff34(int val, int reg) {
	conv_params[reg].coeffs[19].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff40(int val, int reg) {
	conv_params[reg].coeffs[20].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff41(int val, int reg) {
	conv_params[reg].coeffs[21].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff42(int val, int reg) {
	conv_params[reg].coeffs[22].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff43(int val, int reg) {
	conv_params[reg].coeffs[23].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::SetCoeff44(int val, int reg) {
	conv_params[reg].coeffs[24].setPackedValue((unsigned short)val);
}

void ConvolutionFilt::UpdateBuffers(void) {
	if(!od_bit)
		if(out.GetBuffLines() == 0)
			UpdateOutputBufferNoWrap();
		else
			UpdateOutputBuffer();
	if(in.GetBuffLines() == 0)
		UpdateInputBufferNoWrap();
	else
		UpdateInputBuffer();
}

void ConvolutionFilt::UpdateBuffersSync(void) {
	if(!od_bit)
		UpdateOutputBufferSync();
	UpdateInputBufferSync();
}

void ConvolutionFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	kernel_size  = conv_params[reg].kernel_size;
	output_clamp = conv_params[reg].output_clamp;
	abs_bit      = conv_params[reg].abs_bit;
	sq_bit       = conv_params[reg].sq_bit;
	accum_bit    = conv_params[reg].accum_bit;
	od_bit       = conv_params[reg].od_bit;
	thr          = conv_params[reg].thr;
	for(int cc = 0; cc < 25; cc++)
		coeffs[cc] = conv_params[reg].coeffs[cc];
}

void ConvolutionFilt::SetUpAndRun(void) {
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

	// At end of frame, save accumulation and count to APB registers and 		
	if (EndOfFrame()) {
		accum_result = accum;
		accum_cnt_result = accum_cnt;

		// Reset local accumulation and count
		accum = 0.0f;
		accum_cnt = 0;

		// Trigger end of frame interrupt
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

void ConvolutionFilt::TryRun(void) {
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

		// At end of frame		
		if (EndOfFrame()) {
			// Save accumulation and count to APB registers and 
			accum_result = accum;
			accum_cnt_result = accum_cnt;

			// Reset local accumulation and count
			accum = 0.0f;
			accum_cnt = 0;

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

void *ConvolutionFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t **      src_i = 0;
	uint8_t *    kernel_i = 0;
	uint8_t *       dst_i = 0;
	uint8_t *packed_dst_i = 0;
	fp16    **      src_f = 0;
	fp16    *    kernel_f = 0;
	fp16    *       dst_f = 0;
	fp16    *packed_dst_f = 0;

	// Allocating array of pointers to input lines, setting input pointers
	// and allocate the internal 2D kernel for convolution
	kernel_f = new fp16[kernel_size*kernel_size];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_i = new uint8_t *[kernel_size];
		kernel_i = new uint8_t[kernel_size*kernel_size];
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

	fp16 out_val;
	fp16 prod[5][5];
	double sumv[4];
	fp16 sum[3];
	int kr, kc;
	int x = 0;

	while (x < width) {
		// Build the 2D kernel
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			// For uint8_t input build then convert to fp16
			buildKernel(src_i, kernel_i, kernel_size, x);
			convertKernel(kernel_i, kernel_f, kernel_size*kernel_size);
		} else
			buildKernel(src_f, kernel_f, kernel_size, x);

		// Convolve input data with the programmable coefficients
		// Multipliers
		switch(kernel_size) {
		case 3 :
			for(kr = 1; kr < 1+kernel_size; kr++)
				for(kc = 1; kc < 1+kernel_size; kc++)
					prod[kr][kc] = kernel_f[(kr-1)*kernel_size + (kc-1)] * coeffs[kr*kernel_lines + kc];
			break;
		case 5 :
			for(kr = 0; kr < kernel_size; kr++)
				for(kc = 0; kc < kernel_size; kc++)
					prod[kr][kc] = kernel_f[kr*kernel_size + kc] * coeffs[kr*kernel_lines + kc];
			break;
		default:
			; // This branch is never entered, only for warning suppression purposes
		}

		// Accumulators - following hackery gives sum associativity match with 
		// RTL. Also, set rounding mode towards zero to match RTL
		unsigned int rnd;
#ifndef WIN32
		rnd = fegetround();
		fesetround(FE_TOWARDZERO);
#else
		_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif
		int i, j, k;
		i = j = k = 0;
		
		while (i < 3) {
			sumv[i] = 0;
			for (int l = 0; l < 8; l++) {

				sumv[i] += fp16(prod[k][(4-j++)]);
				if (j == 5) {
					j = 0;
					k++;
				}
			}
			i++;
		}
		
		sum[0] = fp16(sumv[0]);
		sum[1] = fp16(sumv[1]);
		sum[2] = fp16(sumv[2]);

		sumv[3] = fp16(prod[4][0]);
		for (i = 0; i < 3; i++)
			sumv[3] += fp16(sum[i]);

		out_val = fp16(sumv[3]);

		// Do the required operation with the convolution result
		fp16 result = out_val;

		if(abs_bit)
			result = fabs(result);
		if(sq_bit)
			result = SQR(result);
				
		// Float accumulation, rounding towards zero
		if (accum_bit) {
			if (result > thr) {
				accum += (float)result;
				accum_cnt++;
			}
		}

		// Restore the rounding mode
#ifndef WIN32
		fesetround(rnd);
#else
		_controlfp_s(0, rnd, _MCW_RC);
#endif

		// Clamp and output (unless disabled)
		if (od_bit == 0) {
			if(output_clamp)
				result = fp16(ClampWR(result, 0.0f, 1.0f));
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_i[x] = FP16_TO_U8F(result);
			else 
				packed_dst_f[x] = result;
		}
		x++;
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		SplitOutputLine(packed_dst_i, width, dst_i);
		delete[] packed_dst_i;
	} else {
		SplitOutputLine(packed_dst_f, width, dst_f);
		delete[] packed_dst_f;
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] kernel_i;
		delete[] src_i;
	} else {
		delete[] src_f;
	}
	delete[] kernel_f;

	return out_ptr;
}
