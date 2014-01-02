// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Luma denoise filter
//
//
// -----------------------------------------------------------------------------

#include "sippLumaDenoise.h"

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


LumaDenoiseFilt::LumaDenoiseFilt(SippIrq *pObj, int id, int k, int rk, std::string name) :
	kernel_size (k),
	ref_kernel_size (rk),
	ref("input", Read) {

		// pObj, name, id and kernel_lines are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);

		ref_max_pad = ref_kernel_size >> 1;
}

void LumaDenoiseFilt::SetBitPosition(int val, int reg) {
	luma_params[reg].bitpos = val;
}

void LumaDenoiseFilt::SetAlpha(int val, int reg) {
	luma_params[reg].alpha = (uint8_t)val;
}

void LumaDenoiseFilt::SetLUT0700(int val, int reg) {
	if((val & 0xf) == 0) {
		std::cerr << "The first LUT entry must not be zero." << std::endl;
		abort();
	}
	luma_params[reg].lut[ 0] = (val >>  0) & 0xf;
	luma_params[reg].lut[ 1] = (val >>  4) & 0xf;
	luma_params[reg].lut[ 2] = (val >>  8) & 0xf;
	luma_params[reg].lut[ 3] = (val >> 12) & 0xf;
	luma_params[reg].lut[ 4] = (val >> 16) & 0xf;
	luma_params[reg].lut[ 5] = (val >> 20) & 0xf;
	luma_params[reg].lut[ 6] = (val >> 24) & 0xf;
	luma_params[reg].lut[ 7] = (val >> 28) & 0xf;
}

void LumaDenoiseFilt::SetLUT1508(int val, int reg) {
	luma_params[reg].lut[ 8] = (val >>  0) & 0xf;
	luma_params[reg].lut[ 9] = (val >>  4) & 0xf;
	luma_params[reg].lut[10] = (val >>  8) & 0xf;
	luma_params[reg].lut[11] = (val >> 12) & 0xf;
	luma_params[reg].lut[12] = (val >> 16) & 0xf;
	luma_params[reg].lut[13] = (val >> 20) & 0xf;
	luma_params[reg].lut[14] = (val >> 24) & 0xf;
	luma_params[reg].lut[15] = (val >> 28) & 0xf;
}

void LumaDenoiseFilt::SetLUT2316(int val, int reg) {
	luma_params[reg].lut[16] = (val >>  0) & 0xf;
	luma_params[reg].lut[17] = (val >>  4) & 0xf;
	luma_params[reg].lut[18] = (val >>  8) & 0xf;
	luma_params[reg].lut[19] = (val >> 12) & 0xf;
	luma_params[reg].lut[20] = (val >> 16) & 0xf;
	luma_params[reg].lut[21] = (val >> 20) & 0xf;
	luma_params[reg].lut[22] = (val >> 24) & 0xf;
	luma_params[reg].lut[23] = (val >> 28) & 0xf;
}

void LumaDenoiseFilt::SetLUT3124(int val, int reg) {
	luma_params[reg].lut[24] = (val >>  0) & 0xf;
	luma_params[reg].lut[25] = (val >>  4) & 0xf;
	luma_params[reg].lut[26] = (val >>  8) & 0xf;
	luma_params[reg].lut[27] = (val >> 12) & 0xf;
	luma_params[reg].lut[28] = (val >> 16) & 0xf;
	luma_params[reg].lut[29] = (val >> 20) & 0xf;
	luma_params[reg].lut[30] = (val >> 24) & 0xf;
	luma_params[reg].lut[31] = (val >> 28) & 0xf;
}

void LumaDenoiseFilt::SetF2LUT(int val, int reg) {
	luma_params[reg].f2_lut = val;
}

// Reference buffer fill level is incremented synchronously with input
int LumaDenoiseFilt::IncInputBufferFillLevel (void) {
	int ret = in.IncFillLevel();
	ref.IncFillLevel();
	TryRun();
	return ret;
}

bool LumaDenoiseFilt::CanRunLuma(int kernel_size, int ref_kernel_size) {
	// At start of frame wait for buffer fill level to hit start line
	if (frame_count == 0 && line_idx == 0 && ( in.GetFillLevel() <  in.GetStartLevel())
		                                  && (ref.GetFillLevel() < ref.GetStartLevel()))
		return false;

	bool can_run;
	if (in.GetBuffLines() == 0) {
		if (out.GetBuffLines() == 0) {
			can_run = true;
		} else {
			can_run = out.GetFillLevel() < out.GetBuffLines();
		}
	} else {
		// Original input padding
		max_pad = kernel_size >> 1;
		if (line_idx < max_pad)
			min_fill = line_idx + max_pad + 1;                // top
		else if (height - line_idx <= max_pad)
			min_fill = max_pad + height - line_idx;           // bottom
		else
			min_fill = kernel_size;                           // middle

		// Reference input padding
		ref_max_pad = ref_kernel_size >> 1;
		if (line_idx < ref_max_pad)
			ref_min_fill = line_idx + ref_max_pad + 1;        // top
		else if (height - line_idx <= ref_max_pad)
			ref_min_fill = ref_max_pad + height - line_idx;   // bottom
		else
			ref_min_fill = ref_kernel_size;                   // middle

		if (out.GetBuffLines() == 0) {
			can_run =  in.GetFillLevel() >=     min_fill &&
	                  ref.GetFillLevel() >= ref_min_fill;
		} else {

			can_run =  in.GetFillLevel() >=     min_fill && out.GetFillLevel() < out.GetBuffLines() &&
			          ref.GetFillLevel() >= ref_min_fill;
		}
	}

	return can_run;
}

void LumaDenoiseFilt::getReferenceLines(uint8_t **ref_ptr, int plane) {
	int pad_lines;

	// Determine in what slice does the current plane start
	ComputeReferenceStartSlice(plane);

	if ((pad_lines = ref_max_pad - line_idx) > 0) {
		// Padding at top
		PadAtTop(ref_ptr, pad_lines, plane);

	} else if ((pad_lines = line_idx + ref_max_pad - (height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(ref_ptr, pad_lines, plane);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(ref_ptr, plane);
	}
}

void LumaDenoiseFilt::PackReferenceLines(uint8_t **split_src, int kernel_size, uint8_t **packed_src) {
	int chunk_size   = ref.GetChunkSize() / ref.GetFormat();
	int slice_stride = g_slice_size       / ref.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)width/chunk_size;

		for(int ln = 0; ln < kernel_size; ln++) {
			uint8_t *l_split_src  = (uint8_t *) split_src[ln];
			uint8_t *l_packed_src = (uint8_t *)packed_src[ln];

			for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
				for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
				l_split_src += slice_stride * NextReferenceSlice(crt_slice + 1);
			}
			if(int remainder = (width - full_slices_no * chunk_size))
				for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
		}
	} else {
		for(int ln = 0; ln < kernel_size; ln++)
			packed_src[ln] = split_src[ln];
	}
}

int LumaDenoiseFilt::NextReferenceSlice(int offset) {
	return (ref_plane_start_slice + offset == g_last_slice + 1) ? g_first_slice - g_last_slice : 1;
}

void LumaDenoiseFilt::ComputeReferenceStartSlice(int plane) {
	if(plane == 0) {
		ref_plane_start_slice = ref.GetStartSlice();
	} else {
		int pstride, sstride, ss_in_ps, wrap;
		pstride  = ref.GetPlaneStride() / ref.GetFormat();
		sstride  = g_slice_size         / ref.GetFormat();
		ss_in_ps = pstride / sstride;

		if((wrap = (ref_plane_start_slice + ss_in_ps - g_last_slice)) > 0)
			ref_plane_start_slice  = g_first_slice + wrap - 1;
		else
			ref_plane_start_slice += ss_in_ps;
	}
}

void LumaDenoiseFilt::PadAtTop(uint8_t **ref_ptr, int pad_lines, int plane) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  ref.GetLineStride () / ref.GetFormat();
	pstride = (ref.GetPlaneStride() / ref.GetFormat()) % g_slice_size;
	sstride = (g_slice_size         / ref.GetFormat()) * ref_plane_start_slice;
	start_slice_address = (uint8_t *)ref.buffer + sstride;

	for ( ; kv < pad_lines; kv++)
		ref_ptr[kv] = start_slice_address 
		     + lstride * ref.GetBufferIdx() 
		     + pstride * plane;

	for ( ; kv < ref_kernel_size; kv++)
		ref_ptr[kv] = start_slice_address 
		     + lstride * ref.GetBufferIdx(kv - pad_lines) 
		     + pstride * plane;
}

void LumaDenoiseFilt::PadAtBottom(uint8_t **ref_ptr, int pad_lines, int plane) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  ref.GetLineStride () / ref.GetFormat();
	pstride = (ref.GetPlaneStride() / ref.GetFormat()) % g_slice_size;
	sstride = (g_slice_size         / ref.GetFormat()) * ref_plane_start_slice;
	start_slice_address = (uint8_t *)ref.buffer + sstride;

	for ( ; kv < ref_kernel_size - pad_lines; kv++)
		ref_ptr[kv] = start_slice_address 
		     + lstride * ref.GetBufferIdx(kv) 
		     + pstride * plane;

	for ( ; kv < ref_kernel_size; kv++)
		ref_ptr[kv] = ref_ptr[kv - 1];
}

void LumaDenoiseFilt::MiddleExecutionV(uint8_t **ref_ptr, int plane) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  ref.GetLineStride () / ref.GetFormat();
	pstride = (ref.GetPlaneStride() / ref.GetFormat()) % g_slice_size;
	sstride = (g_slice_size         / ref.GetFormat()) * ref_plane_start_slice;
	start_slice_address = (uint8_t *)ref.buffer + sstride;

	for ( ; kv < ref_kernel_size; kv++)
		ref_ptr[kv] = start_slice_address 
		     + lstride * ref.GetBufferIdx(kv) 
		     + pstride * plane;
}

void LumaDenoiseFilt::UpdateBuffers(void) {
	if(out.GetBuffLines() == 0)
		UpdateOutputBufferNoWrap();
	else
		UpdateOutputBuffer();
	if(in.GetBuffLines() == 0) {
		UpdateInputBufferNoWrap();
		UpdateReferenceBufferNoWrap();
	} else {
		UpdateInputBuffer();
		UpdateReferenceBuffer();
	}
}

void LumaDenoiseFilt::UpdateBuffersSync(void) {
	UpdateOutputBufferSync();
	UpdateInputBufferSync();
	UpdateReferenceBufferSync();
}

void LumaDenoiseFilt::UpdateReferenceBuffer(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= ref_max_pad; i++) {
			ref.DecFillLevel();
			ref.IncBufferIdx();
		}
	} else if (ref_max_pad - line_idx <= 0) {
		// Not padding at top - decrement input buffer fill level, increment
		// input buffer line index and set interrupt status
		ref.DecFillLevel();
		ref.IncBufferIdx();
	}
}

void LumaDenoiseFilt::UpdateReferenceBufferSync(void) {
	if (line_idx == (height - 1))
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= ref_max_pad; i++)
			ref.IncBufferIdx();
	else if (ref_max_pad - line_idx <= 0)
		// Not padding at top - decrement input buffer fill level, increment
		// input buffer line index and set interrupt status
		ref.IncBufferIdx();
}

void LumaDenoiseFilt::UpdateReferenceBufferNoWrap(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= ref_max_pad; i++)
			ref.IncBufferIdxNoWrap(height);
	} else if (ref_max_pad - line_idx <= 0) {
		// Not padding at top - increment input buffer line index
		ref.IncBufferIdxNoWrap(height);
	}
}

void LumaDenoiseFilt::buildRefKernel(uint8_t **src, uint8_t *kernel, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < ref_max_pad)
		pad_pixels = ref_max_pad - x;
	else if(x > width - ref_max_pad - 1)
		pad_pixels = (width - 1) - (x + ref_max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, x);
}

void LumaDenoiseFilt::PadAtLeft(uint8_t **src, uint8_t *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*ref_kernel_size + kh] = src[kv][0];

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = pad_pixels; kh < ref_kernel_size; kh++)
			kernel[kv*ref_kernel_size + kh] = src[kv][crt_pixel + kh - ref_max_pad];
}

void LumaDenoiseFilt::PadAtRight(uint8_t **src, uint8_t *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = 0; kh < ref_kernel_size - pad_pixels; kh++)
			kernel[kv*ref_kernel_size + kh] = src[kv][crt_pixel + kh - ref_max_pad];

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = ref_kernel_size - pad_pixels; kh < ref_kernel_size; kh++)
			kernel[kv*ref_kernel_size + kh] = kernel[kv*ref_kernel_size + kh - 1];
}

void LumaDenoiseFilt::MiddleExecutionH(uint8_t **src, uint8_t *kernel, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = 0; kh < ref_kernel_size; kh++)
			kernel[kv*ref_kernel_size + kh] = src[kv][crt_pixel + kh - ref_max_pad];
}

// SAD computation
void LumaDenoiseFilt::ComputeSAD(uint8_t *ref, uint16_t *sad, int kernel_size, int sv, int sh)
{
	int ln, col, w_ln, w_col;
	int cntv, cnth;
	int abs_diff;
	int sad_temp = 0;

	for(w_ln = sv, ln = 3, cntv = 0; cntv < 5; w_ln ++, ln ++, cntv++)
		for(w_col = sh, col = 3, cnth = 0; cnth < 5; w_col++, col++, cnth++) {
			abs_diff = abs(ref[ln * kernel_size + col] - ref[w_ln * kernel_size + w_col]);
			abs_diff <<= GetMsb(abs_diff);
			sad_temp += abs_diff;
		}
	// Limit the SAD
	if(sad_temp > 0xffff)
		sad_temp = 0xffff;

	*sad = (uint16_t)sad_temp;
}

void LumaDenoiseFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	ref.SelectBufferParameters();
	bitpos = luma_params[reg].bitpos;
	alpha  = luma_params[reg].alpha;
	f2_lut = luma_params[reg].f2_lut;
	for(int le = 0; le < 32; le++)
		lut[le] = luma_params[reg].lut[le];
}

void LumaDenoiseFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t ** split_in_ptr_i = 0;
	uint8_t **       in_ptr_i = 0;
	fp16    ** split_in_ptr_f = 0;
	fp16    **       in_ptr_f = 0;
	uint8_t **split_ref_ptr   = 0;
	uint8_t **      ref_ptr   = 0;
	uint8_t *       out_ptr_i = 0;
	fp16    *       out_ptr_f = 0;

	void **in_ptr;
	void *out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to original and reference split 
	// input lines and allocate pointers to hold packed incoming data
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
	split_ref_ptr = new uint8_t *[ref_kernel_size];
	      ref_ptr = new uint8_t *[ref_kernel_size];
	for(int ln = 0; ln < ref_kernel_size; ln++)
			ref_ptr[ln] = new uint8_t[width];

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
		getReferenceLines (split_ref_ptr, pl);
		PackReferenceLines(split_ref_ptr, ref_kernel_size, ref_ptr);
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			setOutputLine(&out_ptr_i, pl);
			out_ptr = (void *)out_ptr_i;
		} else {
			setOutputLine(&out_ptr_f, pl);
			out_ptr = (void *)out_ptr_f;
		}
		Run(in_ptr, (void **)ref_ptr, out_ptr);
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
	delete[] split_ref_ptr;
	delete[]       ref_ptr;
}

void LumaDenoiseFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t ** split_in_ptr_i = 0;
	uint8_t **       in_ptr_i = 0;
	fp16    ** split_in_ptr_f = 0;
	fp16    **       in_ptr_f = 0;
	uint8_t **split_ref_ptr   = 0;
	uint8_t **      ref_ptr   = 0;
	uint8_t *       out_ptr_i = 0;
	fp16    *       out_ptr_f = 0;

	void **in_ptr;
	void *out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to original and reference split 
	// input lines and allocate pointers to hold packed incoming data
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
	split_ref_ptr = new uint8_t *[ref_kernel_size];
	      ref_ptr = new uint8_t *[ref_kernel_size];
	for(int ln = 0; ln < ref_kernel_size; ln++)
			ref_ptr[ln] = new uint8_t[width];

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or
	// DecOutputBufferFillLevel() was called...
	while (CanRunLuma(kernel_size, ref_kernel_size)) {
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
			getReferenceLines (split_ref_ptr, pl);
			PackReferenceLines(split_ref_ptr, ref_kernel_size, ref_ptr);
			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				setOutputLine(&out_ptr_i, pl);
				out_ptr = (void *)out_ptr_i;
			} else {
				setOutputLine(&out_ptr_f, pl);
				out_ptr = (void *)out_ptr_f;
			}
			Run(in_ptr, (void **)ref_ptr, out_ptr);
		}
		// Update buffer fill levels and input frame line index
		UpdateBuffers();
		IncLineIdx();

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

	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] split_in_ptr_i;
		delete[]       in_ptr_i;
	} else {
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
	}
	delete[] split_ref_ptr;
	delete[]       ref_ptr;
}

void LumaDenoiseFilt::Run(void **in_ptr_orig, void **in_ptr_ref, void *out_ptr) {
	uint8_t **   orig_src_i = 0;
	uint16_t *orig_kernel_i = 0;
	uint8_t *         dst_i = 0;
	uint8_t *  packed_dst_i = 0;
	uint8_t **    ref_src   = 0;
	uint8_t *  ref_kernel   = 0;
	fp16 **      orig_src_f = 0;
	fp16 *    orig_kernel_f = 0;
	fp16 *            dst_f = 0;
	fp16 *     packed_dst_f = 0;

	// Allocating array of pointers to original lines, setting 
	// original pointers and allocating the original 2D kernel
	orig_kernel_i = new uint16_t[kernel_size*kernel_size];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		orig_src_i = new uint8_t *[kernel_size];
		for (int i = 0; i < kernel_size; i++) {
			orig_src_i[i] = (uint8_t *)in_ptr_orig[i];
		}
	} else {
		orig_src_f = new fp16 *[kernel_size];
		for (int i = 0; i < kernel_size; i++) {
			orig_src_f[i] = (fp16 *)in_ptr_orig[i];
		}
		orig_kernel_f = new fp16[kernel_size*kernel_size];
	}

	// Allocating array of pointers to reference lines
	ref_src = new uint8_t *[ref_kernel_size];

	// Setting reference pointers
	for (int i = 0; i < ref_kernel_size; i++) {
		ref_src[i] = (uint8_t *)in_ptr_ref [i];
	}

	// Allocate the reference 2D kernel
	ref_kernel = new uint8_t[ref_kernel_size*ref_kernel_size];

	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_i = (uint8_t *)out_ptr;
		packed_dst_i = new uint8_t[width];
	} else {
		       dst_f = (fp16    *)out_ptr;
		packed_dst_f = new fp16   [width];
	}

	int x = 0;

	uint16_t sad;
	uint16_t wt;
	uint16_t wt_tmp;
	uint32_t acc;
	uint16_t central_in;
	uint16_t shifted_in;
	int slide_h, slide_v;
	int lut_index;
	int f2_val, f2_index;
	uint16_t denoised;

	// Vertical pass
	while(x < width) {
		// Reset weight and accumulator
		acc = wt = 0;

		// Build the 2D original kernel
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			buildKernel(orig_src_i, orig_kernel_i, kernel_size, x);
		} else {
			buildKernel(orig_src_f, orig_kernel_f, kernel_size, x);
			convertKernel(orig_kernel_f, orig_kernel_i, kernel_size*kernel_size);
		}
		// Build the 2D reference kernel
		buildRefKernel(ref_src, ref_kernel, x);

		// Fetch the current original pixel
		central_in = orig_kernel_i[O_KERNEL_CENTRE];

		// Slide the SAD window to determine similarities between central and neighbour pixels
		for(slide_v = 0; slide_v < 7; slide_v++)
			for(slide_h = 0; slide_h < 7; slide_h++) {

				// Fetch the shifted original pixel
				shifted_in = orig_kernel_i[slide_v * kernel_size + slide_h];

				// Reset and compute the sum of absolute differences
				sad = 0;
				ComputeSAD(ref_kernel, &sad, ref_kernel_size, slide_v, slide_h);

				// Prepare the LUT index
				sad >>= bitpos;
				lut_index = (sad <= 31) * (sad & 0x1f);

				// Compute the F2 coefficient
				if(slide_v < 4) {
					if(slide_h < 4) {
						f2_index = (slide_v << 3) + (slide_h << 1);
					} else {
						f2_index = (slide_v << 3) + ((6 - slide_h) << 1);
					}
				} else {
					if(slide_h < 4) {
						f2_index = ((6 - slide_v) << 3) + (slide_h << 1);
					} else {
						f2_index = ((6 - slide_v) << 3) + ((6 - slide_h) << 1);
					}
				}
				f2_val = 1 << ((f2_lut & (F2_MASK << f2_index)) >> f2_index);

				// Accumulate weigths and pixels
				wt_tmp = (uint16_t)((sad <= 31) * lut[lut_index] * f2_val);
				wt  += wt_tmp;
				acc += wt_tmp * shifted_in;
			}

		// Control the denoising's aggression and output
        denoised = (uint16_t)(acc / wt);
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			uint8_t out_pixel_u8 = (uint8_t)(U12F_TO_U8F(denoised) * fp16((float)alpha/0x80) + (1 - fp16((float)alpha/0x80)) * U12F_TO_U8F(central_in));
			packed_dst_i[x] = out_pixel_u8;
		} else {
			uint16_t out_pixel_u12 = (denoised * alpha + (0x80 - alpha) * central_in) / (0x80);
			packed_dst_f[x] = U12F_TO_FP16(out_pixel_u12);
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

	delete[]  ref_kernel;
	delete[] orig_kernel_i;
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		delete[] orig_src_i;
	} else {
		delete[] orig_kernel_f;
		delete[] orig_src_f;
	}
	delete[] ref_src;
}

// Not used in this particular filter as it has his own version of Run
void *LumaDenoiseFilt::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}
