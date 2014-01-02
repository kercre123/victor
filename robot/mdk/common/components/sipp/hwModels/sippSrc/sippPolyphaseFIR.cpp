// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Polyphase FIR filter
//
//
// -----------------------------------------------------------------------------

#include "sippPolyphaseFIR.h"
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
#include <math.h>
#include <float.h>

// Need to set rounding mode to match HW
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif

PolyphaseFIRFilt::PolyphaseFIRFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k),
	v_numerator (1),
	v_denominator (1),
	h_numerator (1),
	h_denominator (1),
	which_line (0),
	v_phase (0) {

	// pObj, name and id are protected members of SippBaseFilter
	SetPIrqObj (pObj);
	SetName (name);
	SetId (id);

	for(int i = 0 ; i < PHASE_NO*COEFFS_PER_PHASE; i++) {
		v_coeffs[i] = 0;
		h_coeffs[i] = 0;
	}

	for(int i = 0 ; i < PHASE_NO; i++)
		v_ph[i] = 0;

	eoof = out_ln_generated = upscale = downscale = false;
	valid = in_ln_consumed = true;
}

void PolyphaseFIRFilt::SetKernelSize(int val, int reg) {
	if(!strchr(upfirdn_kernel_sizes, (char)(val + '0')))
		std::cerr << "Invalid kernel size: possible sizes are 3, 5 or 7." << std::endl;
	else {
		upfirdn_params[reg].kernel_size = val;
		base_params[reg].max_pad = val >> 1;
		coeff_offset = (val == 7) ? 0 : (val == 5) ? 1 : 2;
	}
}

void PolyphaseFIRFilt::SetOutputClamp(int val, int reg) {
    upfirdn_params[reg].output_clamp = (out.GetFormat() == SIPP_FORMAT_8BIT) ? 1 : val;
}

void PolyphaseFIRFilt::SetVerticalNumerator(int val, int reg) {
	if(val == 0 || val > 16) {
		std::cerr << "Invalid vertical numerator: must be in range [1 - 16]." << std::endl;
		abort();
	}
	else {
		upfirdn_params[reg].v_numerator = val;
		d_count = upfirdn_params[reg].v_numerator - 1;
	}
}

void PolyphaseFIRFilt::SetVerticalDenominator(int val, int reg) {
	if(val == 0 || val > 63) {
		std::cerr << "Invalid vertical denominator: must be in range [1 - 63]." << std::endl;
		abort();
	}
	else
		upfirdn_params[reg].v_denominator = val;
}

void PolyphaseFIRFilt::SetHorizontalNumerator(int val, int reg) {
	if(val == 0 || val > 16) {
		std::cerr << "Invalid horizontal numerator: must be in range [1 - 16]." << std::endl;
		abort();
	}
	else
		upfirdn_params[reg].h_numerator = val;
}

void PolyphaseFIRFilt::SetHorizontalDenominator(int val, int reg) {
	if(val == 0 || val > 63) {
		std::cerr << "Invalid horizontal denominator: must be in range [1 - 63]." << std::endl;
		abort();
	}
	else
		upfirdn_params[reg].h_denominator = val;
}

void PolyphaseFIRFilt::SetVerticalPhase(int val) {
	v_phase = val;
}

void PolyphaseFIRFilt::SetVerticalDecimationCounter(int val) {
	d_count = val;
}

void PolyphaseFIRFilt::SetVCoeff(int n, int val, int crt_phase, int reg) {
	if (n < 0 || n > 6) {
		std::cerr << "Invalid vertical coefficient index: must be 0 < n <= 6" << std::endl;
		abort();
	}
	if (n < 7)
		upfirdn_params[reg].v_coeffs[crt_phase*COEFFS_PER_PHASE + n].setPackedValue((uint16_t)val);
}

void PolyphaseFIRFilt::SetHCoeff(int n, int val, int crt_phase, int reg) {
	if (n < 0 || n > 6) {
		std::cerr << "Invalid horizontal coefficient index: must be 0 < n <= 6" << std::endl;
		abort();
	}
	if (n < 7)
		upfirdn_params[reg].h_coeffs[crt_phase*COEFFS_PER_PHASE + n].setPackedValue((uint16_t)val);
}

void PolyphaseFIRFilt::UpdateBuffers() {
	if(out_ln_generated)
		if(out.GetBuffLines() == 0)
			UpdateOutputBufferNoWrap();
		else
			UpdateOutputBuffer();
	if(in_ln_consumed)
		if(in.GetBuffLines() == 0) {
			UpdateInputBufferNoWrap();
		} else {
			UpdateInputBuffer();
		}
}

void PolyphaseFIRFilt::UpdateOutputBufferNoWrap() {
	// Increment output buffer line index
	out.IncBufferIdxNoWrap(height_o);
	// At the end of the frame
	if(out.GetBufferIdx() == 0)
		// Set interrupt status
		OutputBufferLevelIncIrq();
}

int PolyphaseFIRFilt::StillToReceive() {
	return GetLineIdx() + max_pad < height - 1;
}

void PolyphaseFIRFilt::DetermineVerticalOperation() {
	if(v_numerator >= v_denominator)
		upscale   = true;
	else
		downscale = true;
}

void PolyphaseFIRFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	kernel_size   = upfirdn_params[reg].kernel_size;
	output_clamp  = upfirdn_params[reg].output_clamp;
	v_numerator   = upfirdn_params[reg].v_numerator;
	v_denominator = upfirdn_params[reg].v_denominator;
	h_numerator   = upfirdn_params[reg].h_numerator;
	h_denominator = upfirdn_params[reg].h_denominator;
	for(int c = 0; c < PHASE_NO*COEFFS_PER_PHASE; c++) {
		v_coeffs[c] = upfirdn_params[reg].v_coeffs[c];
		h_coeffs[c] = upfirdn_params[reg].h_coeffs[c];
	}
}

void PolyphaseFIRFilt::SetUpAndRun(void) {
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

	// Determine whether the filter is up or downscaling vertically
	if(line_idx == 0)
		DetermineVerticalOperation();

	// Determine whether the current buffer line is valid
	if(downscale) {
		if(d_count >= v_numerator) {
			valid = false;
		} else {
			valid = true;
		}
	}

	if(valid) {
		// Assign first dimension of pointers to split input lines
		// and allocate pointers to hold packed incoming data
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			split_in_ptr_i = new uint8_t *[kernel_size];
			      in_ptr_i = new uint8_t *[kernel_size];
			for(int ln = 0; ln < kernel_size; ln++)
				in_ptr_i[ln] = new uint8_t[width];
			in_ptr = (void **)in_ptr_i;
		} else {
			split_in_ptr_f = new fp16    *[kernel_size];
			      in_ptr_f = new fp16    *[kernel_size];
			for(int ln = 0; ln < kernel_size; ln++)
				in_ptr_f[ln] = new fp16   [width];
			in_ptr = (void **)in_ptr_f;
		}

		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << " (V phase = " << v_phase << ")" << std::endl;

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

			RunSync(in_ptr, out_ptr);
		}
		// Update output buffer fill level
		UpdateOutputBufferSync();
	}
	// Clear start bit, update input buffer fill level and 
	// input frame line index based on scaling factors
	in.ClrStartBit();
	if(upscale) {
		if (v_phase +  v_denominator >= v_numerator) {
			v_phase += v_denominator -  v_numerator;
			UpdateInputBufferSync();
			IncLineIdx();

			// Trigger end of frame interrupt
			if (EndOfFrame()) {
				EndOfFrameIrq();
			}
		} else {
			v_phase += v_denominator;
		}
	}
	if(downscale) {
		UpdateInputBufferSync();
		IncLineIdx();
		if (d_count +  v_numerator >= v_denominator) {
			d_count += v_numerator -  v_denominator;
			v_phase  = v_numerator -  d_count - 1;
		} else {
			d_count += v_numerator;
		}

		// Trigger end of frame interrupt
		if (EndOfFrame()) {
			EndOfFrameIrq();
		}
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] split_in_ptr_i;
		delete[]       in_ptr_i;
	} else {
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
	}
}

void PolyphaseFIRFilt::TryRun(void) {
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
	if(!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointer to input lines
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		in_ptr_i = new uint8_t *[kernel_size];
		in_ptr = (void **)in_ptr_i;
	} else {
		in_ptr_f = new fp16 *[kernel_size];
		in_ptr = (void **)in_ptr_f;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or
	// DecOutputBufferFillLevel() was called..
	while (CanRun(kernel_size)) {
		// Set filter output buffer pointer
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			out_ptr_i = (uint8_t *)((uint8_t *)out.buffer + out.GetOffset());
			out_ptr = (void *)out_ptr_i;
		} else {
			out_ptr_f = (fp16 *)((fp16 *)out.buffer + out.GetOffset());
			out_ptr = (void *)out_ptr_f;
		}

		// Run filter
		Run(in_ptr, out_ptr);

		// At end of frame		
		if (EndOfFrame()) {
			// Disable the filter unless the
			// input buffer is circular
			if(in.GetBuffLines() == 0) {
				Disable();
				break;
			}
		}
	}
	cs.Leave();

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] in_ptr_i;
	else
		delete[] in_ptr_f;
}

void PolyphaseFIRFilt::RunSync(void **in_ptr, void *out_ptr) {
	uint8_t **      src_i = 0;
	fp16    **      src_f = 0;
	fp16    *      kernel = 0;
	uint8_t *       dst_i = 0;
	uint8_t *packed_dst_i = 0;
	fp16    *       dst_f = 0;
	fp16    *packed_dst_f = 0;

	// Allocating array of pointers to input lines, setting input pointers
	// and allocate internal kernel on which the processing and padding are done
	kernel = new fp16[kernel_size];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_i = new uint8_t *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			src_i[i] = (uint8_t *)in_ptr[i];
	} else {
		src_f = new fp16    *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			src_f[i] = (fp16    *)in_ptr[i];
	}

	// Output of vertical upscaling and low filter pass
	fp16 *upscaled_v_kernel = new fp16 [width];

	// Kernel for horizontal upscaling and low filter pass
	fp16 *upscaled_h_kernel = new fp16[h_numerator];

	// Double precision variables for filter accumulations to give sum associativity match with RTL
	double uvk, uhk;

	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_i = (uint8_t *)out_ptr;
		packed_dst_i = new uint8_t[width_o];
	} else {
		       dst_f = (fp16    *)out_ptr;
		packed_dst_f = new fp16   [width_o];
	}
	
	// Set rounding mode towards zero to match RTL
	unsigned int rnd;
#ifndef WIN32
	rnd = fegetround();
	fesetround(FE_TOWARDZERO);
#else
	_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif
	
	int x, h_ph, which_pixel, out_idx;
	x = 0;
	while (x < width) {
		// Vertical upscale and low pass filter
		uvk = 0;
		for(int line = 0; line < kernel_size; line++)
			if(in.GetFormat() == SIPP_FORMAT_8BIT)
				uvk += fp16(U8F_TO_FP16(src_i[line][x]) * v_coeffs[v_phase*COEFFS_PER_PHASE + line + coeff_offset]);
			else
				uvk +=             fp16(src_f[line][x]  * v_coeffs[v_phase*COEFFS_PER_PHASE + line + coeff_offset]);
		upscaled_v_kernel[x] = fp16(uvk);

		x++;
	}

	// Horizontal pass
	x = which_pixel = out_idx = 0;
	while (x < width) {
		// Horizontal upscale and low pass filter
		buildKernel(upscaled_v_kernel, kernel, kernel_size, x);

		for(int ph = 0; ph < h_numerator; ph++) {
			uhk = 0;
			for(int pix = 0; pix < kernel_size; pix++) 
				uhk += fp16(kernel[pix] * h_coeffs[ph*COEFFS_PER_PHASE + pix + coeff_offset]);
			upscaled_h_kernel[ph] = fp16(uhk);
		}

		// Horizontal downscaling
		for(h_ph = which_pixel; h_ph < h_numerator; h_ph += h_denominator) {
			if(output_clamp)
				upscaled_h_kernel[h_ph] = fp16(ClampWR(upscaled_h_kernel[h_ph], 0.0f, 1.0f));

			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_i[out_idx++] = FP16_TO_U8F(upscaled_h_kernel[h_ph]);
			else
				packed_dst_f[out_idx++] =        fp16(upscaled_h_kernel[h_ph]);
		}
		x++;

		// Skip over pixels not contributing to output line
		while(h_ph >= (h_numerator << 1)) {
			x++;
			h_ph -= h_numerator;
		}

		which_pixel = h_ph % h_numerator;
	}

	// Restore the rounding mode
#ifndef WIN32
	fesetround(rnd);
#else
	_controlfp_s(0, rnd, _MCW_RC);
#endif

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		SplitOutputLine(packed_dst_i, width_o, dst_i);
		delete[] packed_dst_i;
	} else {
		SplitOutputLine(packed_dst_f, width_o, dst_f);
		delete[] packed_dst_f;
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] src_i;
	else
		delete[] src_f;

	delete[] upscaled_v_kernel;
	delete[]            kernel;
	delete[] upscaled_h_kernel;
}

void *PolyphaseFIRFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t **split_src_i = 0;
	uint8_t **      src_i = 0;
	fp16    **split_src_f = 0;
	fp16    **      src_f = 0;
	fp16    *      kernel = 0;
	uint8_t *       dst_i = 0;
	uint8_t *packed_dst_i = 0;
	fp16    *       dst_f = 0;
	fp16    *packed_dst_f = 0;

	// Double precision variables for filter accumulations to give sum associativity match with RTL
	double uvk, uhk;

	fp16 **upscaled_v_kernel;
	fp16 *upscaled_h_kernel;

	static int count = 0;

	// End of output frame reached
	if (eoof) {		
		// The entire input frame is filtered
		if(EndOfFrame()) {
			// Reset the end of output frame flag
			eoof = false;

			// Trigger end of frame interrupt
			EndOfFrameIrq();

			return 0;
		}

		// Wait to reach the end of input frame as well
		if (StillToReceive()) {
			UpdateBuffers();
			IncLineIdx();

			return 0;
		}

		// Flush any remaining lines from the bottom of the previous input 
		// frame out of the input buffer
		while (!EndOfFrame()) {
			UpdateBuffers();
			IncLineIdx();
		}

		// Reset the end of output frame flag
		eoof = false;

		// Trigger end of frame interrupt
		EndOfFrameIrq();

		return 0;
	}

	// Assign first dimension of pointers to split input lines,
	// allocate pointers to hold packed incoming data and allocate the
	// internal kernel on which the processing and padding are done
	kernel = new fp16[kernel_size];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_src_i = new uint8_t *[kernel_size];
		      src_i = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			src_i[ln] = new uint8_t[width];
	} else {
		split_src_f = new fp16    *[kernel_size];
		      src_f = new fp16    *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			src_f[ln] = new fp16   [width];
	}

	// Output of vertical upscaling and low filter pass
	upscaled_v_kernel = new fp16 *[v_numerator];
	for(int i = 0; i < v_numerator; i++)
		upscaled_v_kernel[i] = new fp16[width];

	// Kernel for horizontal upscaling and low filter pass
	upscaled_h_kernel = new fp16[h_numerator];

	int x, h_ph, which_pixel, pl, out_idx;

	do {
        bool skip = false;
		// Loop over planes
		for (pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Consume input buffer lines not contributing to output frame
			if (v_ph[pl] >= (v_numerator << 1)) {
				v_ph[pl] -= v_numerator;

				if (pl == in.GetNumPlanes()) {
					skip = true;
					break;
				} else
					continue;
			}

			// Compute the vertical downscaling decision variables
			which_line = v_ph[pl] % v_numerator;
			v_ph[pl] = which_line;

            if (pl == in.GetNumPlanes() && Verbose())
				std::cout << name << " run " << std::setbase(10) << count++ << " (V phase = " << which_line << ")" << std::endl;

			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_src_i, kernel_size, pl);
				PackInputLines(split_src_i, kernel_size, src_i);
			} else {
				getKernelLines(split_src_f, kernel_size, pl);
				PackInputLines(split_src_f, kernel_size, src_f);
			}

			// Set rounding mode towards zero to match RTL
			unsigned int rnd;
#ifndef WIN32
			rnd = fegetround();
			fesetround(FE_TOWARDZERO);
#else
			_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif
            x = 0;
			while (x < width) {
				// Vertical upscale and low pass filter
				for(int ph = 0; ph < v_numerator; ph++) {
					uvk = 0;
					for(int line = 0; line < kernel_size; line++) {
						if(in.GetFormat() == SIPP_FORMAT_8BIT) 
							uvk += fp16(U8F_TO_FP16(src_i[line][x]) * v_coeffs[ph*COEFFS_PER_PHASE + line + coeff_offset]);
						else
							uvk +=             fp16(src_f[line][x]  * v_coeffs[ph*COEFFS_PER_PHASE + line + coeff_offset]);
					upscaled_v_kernel[ph][x] = fp16(uvk);
					}
				}
				x++;
			}

			// Setting output pointer at current output buffer line 
			// index and allocating temporary packed output buffer
			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				       dst_i = (uint8_t *)out_ptr + (out.GetLineStride()  / out.GetFormat()) * out.GetBufferIdx() +
				                                    (out.GetPlaneStride() / out.GetFormat()) * pl;
				packed_dst_i = new uint8_t[width_o];
			} else {
				       dst_f = (fp16    *)out_ptr + (out.GetLineStride()  / out.GetFormat()) * out.GetBufferIdx() +
				                                    (out.GetPlaneStride() / out.GetFormat()) * pl;
				packed_dst_f = new fp16   [width_o];
			}

			// Horizontal pass
            x = which_pixel = out_idx = 0;
			while (x < width) {
				// Horizontal upscale and low pass filter
				buildKernel(upscaled_v_kernel[which_line], kernel, kernel_size, x);

				for(int ph = 0; ph < h_numerator; ph++) {
					uhk = 0;
					for(int pix = 0; pix < kernel_size; pix++)
						uhk += fp16(kernel[pix] * h_coeffs[ph*COEFFS_PER_PHASE + pix + coeff_offset]);
					upscaled_h_kernel[ph] = fp16(uhk);
				}

				// Horizontal downscaling
				for(h_ph = which_pixel; h_ph < h_numerator; h_ph += h_denominator) {
					if(output_clamp)
						upscaled_h_kernel[h_ph] = fp16(ClampWR(upscaled_h_kernel[h_ph], 0.0f, 1.0f));

					if(out.GetFormat() == SIPP_FORMAT_8BIT)
						packed_dst_i[out_idx++] = FP16_TO_U8F(upscaled_h_kernel[h_ph]);
					else
						packed_dst_f[out_idx++] =        fp16(upscaled_h_kernel[h_ph]);
				}
				x++;

				// Skip over pixels not contributing to output line
				while(h_ph >= (h_numerator << 1)) {
					x++;
					h_ph -= h_numerator;
				}

				which_pixel = h_ph % h_numerator;
			}

			// Restore the rounding mode
#ifndef WIN32
			fesetround(rnd);
#else
			_controlfp_s(0, rnd, _MCW_RC);
#endif
			v_ph[pl] += v_denominator;

			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				SplitOutputLine(packed_dst_i, width_o, dst_i);
				delete[] packed_dst_i;
			} else {
				SplitOutputLine(packed_dst_f, width_o, dst_f);
				delete[] packed_dst_f;
			}
		}

		// Increment input buffer fill level and input line index
		// when the lines don't contribute to the output frame
		if (skip) {
			UpdateBuffers();
			IncLineIdx();
			continue;
		}

		// When a line of output was generated from each plane,
		// set the flag to update output buffer fill levels and line
		// index, but don't consume the current line unless necessary
		out_ln_generated = true;
		in_ln_consumed = false;
		UpdateBuffers();

		// The output buffer was updated, reset the flags
		out_ln_generated = false;
		in_ln_consumed = true;

		// Increment input buffer fill level and input
		// line index when all the phases are done
		if (v_ph[pl - 1] >= v_numerator) {
			UpdateBuffers();
			IncLineIdx();
		}		

		if (IncOutputLineIdx() == 0) {
			// End of output frame (see above)
			eoof = true;

			// Reset vertical phase at end of output frame
			for(int plane_no = 0; plane_no <= in.GetNumPlanes(); plane_no++)
				v_ph[plane_no] = 0;

			which_line = 0;
			break;
		}
	} while (CanRun(kernel_size));

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] split_src_i;
		delete[]       src_i;
	} else {
		delete[] split_src_f;
		delete[]       src_f;
	}

	for(int i = 0; i < v_numerator; i++)
		delete[] upscaled_v_kernel[i];
	delete[] upscaled_v_kernel;
	delete[]            kernel;
	delete[] upscaled_h_kernel;

	return 0;
}
