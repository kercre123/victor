// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Color combination filter
//
//
// -----------------------------------------------------------------------------

#include "sippColorCombination.h"
#include "sippDebug.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>


ColorCombinationFilt::ColorCombinationFilt(SippIrq *pObj, int id, int k, int ck, std::string name) :
	kernel_size (k),
	chr_kernel_size (ck),
	chr ("input", Read) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);

		chr_max_pad = chr_kernel_size >> 1;
		chr_inc_en  = chr_line_idx = 0;

		for(int i = 0; i < chr_kernel_size - 1; i++) {
			even_coeffs[i] = kern1[i];
			 odd_coeffs[i] = kern2[i];
		}

		// Mask the luma interest bits regarding range stops
		luma_used_bits = 8;
		luma_shift_amount = 12 - luma_used_bits;
		luma_mask = ((1 << luma_used_bits) - 1) << luma_shift_amount;
}

void ColorCombinationFilt::SetForceLumaOne(int val) {
	force_luma_one = val;
	if(force_luma_one)
		crt_conv_luma = 0xFFF;
}

void ColorCombinationFilt::SetMulCoeff(int val) {
	mul = val;
}

void ColorCombinationFilt::SetThreshold(int val) {
	t1 = val;
	conv_t1 = U8F_TO_U12F(t1);
}

void ColorCombinationFilt::SetPlaneMultiple(int val) {
	plane_multiple = val;

	int actual_np_o = out.GetNumPlanes() + 1;
	int actual_pm   = plane_multiple     + 1;
	full_set  = actual_np_o / actual_pm;
	remainder = actual_np_o - actual_pm * full_set;
}

void ColorCombinationFilt::SetRPlaneCoeff(int val) {
	k_rgb[0].full = val;
}

void ColorCombinationFilt::SetGPlaneCoeff(int val) {
	k_rgb[1].full = val;
}

void ColorCombinationFilt::SetBPlaneCoeff(int val) {
	k_rgb[2].full = val;
}

void ColorCombinationFilt::SetEpsilon(int val) {
	epsilon = val;
}

void ColorCombinationFilt::SetCCM00(int val) {
	ccm[0].full = val;
}

void ColorCombinationFilt::SetCCM01(int val) {
	ccm[1].full = val;
}

void ColorCombinationFilt::SetCCM02(int val) {
	ccm[2].full = val;
}

void ColorCombinationFilt::SetCCM10(int val) {
	ccm[3].full = val;
}

void ColorCombinationFilt::SetCCM11(int val) {
	ccm[4].full = val;
}

void ColorCombinationFilt::SetCCM12(int val) {
	ccm[5].full = val;
}

void ColorCombinationFilt::SetCCM20(int val) {
	ccm[6].full = val;
}

void ColorCombinationFilt::SetCCM21(int val) {
	ccm[7].full = val;
}

void ColorCombinationFilt::SetCCM22(int val) {
	ccm[8].full = val;
}

// Chroma buffer fill level is incremented synchronously with input, but only on every other increment
int ColorCombinationFilt::IncInputBufferFillLevel (void) {
	int ret = in.IncFillLevel();
	if(++chr_inc_en % 2)
		chr.IncFillLevel();
	if(chr_inc_en == height)
		chr_inc_en = 0;
	TryRun();
	return ret;
}

bool ColorCombinationFilt::CanRunCc(int kernel_size, int chr_kernel_size) {
	// At start of frame wait for buffer fill level to hit start line
	if (frame_count == 0 && line_idx == 0 && ( in.GetFillLevel() <  in.GetStartLevel())
		                                  && (chr.GetFillLevel() < chr.GetStartLevel()))
		return false;

	bool can_run;
	if (in.GetBuffLines() == 0) {
		if (out.GetBuffLines() == 0) {
			can_run = true;
		} else {
			can_run = out.GetFillLevel() < out.GetBuffLines();
		}
	} else {
		// Luma input padding
		max_pad = kernel_size >> 1;
		if (line_idx < max_pad)
			min_fill = line_idx + max_pad + 1;                      // top
		else if (height - line_idx <= max_pad)					    
			min_fill = max_pad + height - line_idx;                 // bottom
		else													    
			min_fill = kernel_size;                                 // middle

		// Chroma input padding
		chr_max_pad = chr_kernel_size >> 1;
		if (chr_line_idx < chr_max_pad)
			chr_min_fill = chr_line_idx + chr_max_pad + 1;          // top
		else if (chr_height - chr_line_idx <= chr_max_pad)
			chr_min_fill = chr_max_pad + chr_height - chr_line_idx; // bottom
		else
			chr_min_fill = chr_kernel_size;                         // middle

		if (out.GetBuffLines() == 0) {
			can_run =  in.GetFillLevel() >=     min_fill &&
	                  chr.GetFillLevel() >= chr_min_fill;
		} else {

			can_run =  in.GetFillLevel() >=     min_fill && out.GetFillLevel() < out.GetBuffLines() &&
			          chr.GetFillLevel() >= chr_min_fill;
		}
	}

	return can_run;
}

void ColorCombinationFilt::getChromaLines(uint8_t ***chr_ptr) {
	int pad_lines;

	if ((pad_lines = chr_max_pad - chr_line_idx) > 0) {
		// Padding at top
		PadAtTop(chr_ptr, pad_lines);

	} else if ((pad_lines = chr_line_idx + chr_max_pad - (chr_height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(chr_ptr, pad_lines);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(chr_ptr);
	}
}

void ColorCombinationFilt::PackChromaLines(uint8_t ***split_src, int kernel_size, uint8_t ***packed_src) {
	int chunk_size   = chr.GetChunkSize() / chr.GetFormat();
	int slice_stride = g_slice_size       / chr.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)chr_width/chunk_size;

		for(int pl = 0; pl <= chr.GetNumPlanes(); pl++)
			for(int ln = 0; ln < kernel_size; ln++) {
				uint8_t *l_split_src  = (uint8_t *) split_src[pl][ln];
				uint8_t *l_packed_src = (uint8_t *)packed_src[pl][ln];

				for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
					for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
						*l_packed_src++ = l_split_src[crt_pixel];
					l_split_src += slice_stride * NextChromaSlice(crt_slice + 1);
				}
				if(int remainder = (chr_width - full_slices_no * chunk_size))
					for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
						*l_packed_src++ = l_split_src[crt_pixel];
			}
	} else {
		for(int pl = 0; pl <= chr.GetNumPlanes(); pl++)
			for(int ln = 0; ln < kernel_size; ln++)
				packed_src[pl][ln] = split_src[pl][ln];
	}
}

int ColorCombinationFilt::NextChromaSlice(int offset) {
	return (chr_plane_start_slice + offset == g_last_slice + 1) ? g_first_slice - g_last_slice : 1;
}

void ColorCombinationFilt::ComputeChromaStartSlice(int plane) {
	if(plane == 0) {
		chr_plane_start_slice = chr.GetStartSlice();
	} else {
		int pstride, sstride, ss_in_ps, wrap;
		pstride  = chr.GetPlaneStride() / chr.GetFormat();
		sstride  = g_slice_size         / chr.GetFormat();
		ss_in_ps = pstride / sstride;

		if((wrap = (chr_plane_start_slice + ss_in_ps - g_last_slice)) > 0)
			chr_plane_start_slice  = g_first_slice + wrap - 1;
		else
			chr_plane_start_slice += ss_in_ps;
	}
}

void ColorCombinationFilt::PadAtTop(uint8_t ***chr_ptr, int pad_lines) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  chr.GetLineStride () / chr.GetFormat();
	pstride = (chr.GetPlaneStride() / chr.GetFormat()) % g_slice_size;
	sstride =  g_slice_size         / chr.GetFormat();

	for(int plane = 0; plane <= chr.GetNumPlanes(); plane++) {
		int kv = 0;
		ComputeChromaStartSlice(plane);
		start_slice_address = (uint8_t *)chr.buffer + sstride * chr_plane_start_slice;

		for ( ; kv < pad_lines; kv++)
			chr_ptr[plane][kv] = start_slice_address 
			     + lstride * chr.GetBufferIdx() 
			     + pstride * plane;

		for ( ; kv < chr_kernel_size; kv++)
			chr_ptr[plane][kv] = start_slice_address 
			     + lstride * chr.GetBufferIdx(kv - pad_lines) 
			     + pstride * plane;
	}
}

void ColorCombinationFilt::PadAtBottom(uint8_t ***chr_ptr, int pad_lines) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  chr.GetLineStride () / chr.GetFormat();
	pstride = (chr.GetPlaneStride() / chr.GetFormat()) % g_slice_size;
	sstride =  g_slice_size         / chr.GetFormat();

	for(int plane = 0; plane <= chr.GetNumPlanes(); plane++) {
		int kv = 0;
		ComputeChromaStartSlice(plane);
		start_slice_address = (uint8_t *)chr.buffer + sstride * chr_plane_start_slice;

		for ( ; kv < chr_kernel_size - pad_lines; kv++)
			chr_ptr[plane][kv] = start_slice_address 
			     + lstride * chr.GetBufferIdx(kv) 
			     + pstride * plane;

		for ( ; kv < chr_kernel_size; kv++)
			chr_ptr[plane][kv] = chr_ptr[plane][kv - 1];
	}
}

void ColorCombinationFilt::MiddleExecutionV(uint8_t ***chr_ptr) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride =  chr.GetLineStride () / chr.GetFormat();
	pstride = (chr.GetPlaneStride() / chr.GetFormat()) % g_slice_size;
	sstride =  g_slice_size         / chr.GetFormat();

	for(int plane = 0; plane <= chr.GetNumPlanes(); plane++) {
		int kv = 0;
		ComputeChromaStartSlice(plane);
		start_slice_address = (uint8_t *)chr.buffer + sstride * chr_plane_start_slice;

		for ( ; kv < chr_kernel_size; kv++)
			chr_ptr[plane][kv] = start_slice_address 
			     + lstride * chr.GetBufferIdx(kv) 
			     + pstride * plane;
	}
}

void ColorCombinationFilt::UpdateBuffers(void) {
	if(out.GetBuffLines() == 0)
		UpdateOutputBufferNoWrap();
	else
		UpdateOutputBuffer();
	if(in.GetBuffLines() == 0) {
		UpdateInputBufferNoWrap();
		if(line_idx % 2)
			UpdateChromaBufferNoWrap();
	} else {
		UpdateInputBuffer();
		if(line_idx % 2)
			UpdateChromaBuffer();
	}
}

void ColorCombinationFilt::UpdateBuffersSync(void) {
	UpdateOutputBufferSync();
	UpdateInputBufferSync();
	if(line_idx % 2)
		UpdateChromaBufferSync();
}

void ColorCombinationFilt::UpdateChromaBuffer(void) {
	if (chr_line_idx == (chr_height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= chr_max_pad; i++) {
			chr.DecFillLevel();
			chr.IncBufferIdx();
		}
	} else if (chr_max_pad - chr_line_idx <= 0) {
		// Not padding at top - decrement input buffer fill level 
		// and increment chroma buffer line index
		chr.DecFillLevel();
		chr.IncBufferIdx();
	}
}

void ColorCombinationFilt::UpdateChromaBufferSync(void) {
	if (chr_line_idx == (chr_height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= chr_max_pad; i++)
			chr.IncBufferIdx();
	} else if (chr_max_pad - chr_line_idx <= 0)
		// Not padding at top - increment chroma buffer line index
		chr.IncBufferIdx();
}

void ColorCombinationFilt::UpdateChromaBufferNoWrap(void) {
	if (chr_line_idx == (chr_height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= chr_max_pad; i++)
			chr.IncBufferIdxNoWrap(chr_height);
	} else if (chr_max_pad - chr_line_idx <= 0) {
		// Not padding at top - increment chroma buffer line index
		chr.IncBufferIdxNoWrap(chr_height);
	}
}

int ColorCombinationFilt::IncChrLineIdx(void) {
	if (++chr_line_idx >= chr_height)
		chr_line_idx = 0;
	return chr_line_idx;
}

void ColorCombinationFilt::buildChromaKernel(int *src, int *kernel, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < chr_max_pad)
		pad_pixels = chr_max_pad - x;
	else if(x > chr_width - chr_max_pad - 1)
		pad_pixels = (chr_width - 1) - (x + chr_max_pad);
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

void ColorCombinationFilt::PadAtLeft(int *src, int *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh = 0;

	for ( ; kh < pad_pixels; kh++)
		kernel[kh] = src[0];

	for ( ; kh < chr_kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - chr_max_pad];
}

void ColorCombinationFilt::PadAtRight(int *src, int *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh = 0;

	for ( ; kh < chr_kernel_size - pad_pixels; kh++)
		kernel[kh] = src[crt_pixel + kh - chr_max_pad];

	for ( ; kh < chr_kernel_size; kh++)
		kernel[kh] = kernel[kh - 1];
}

void ColorCombinationFilt::MiddleExecutionH(int *src, int *kernel, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh = 0;

	for ( ; kh < chr_kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - chr_max_pad];
}

void ColorCombinationFilt::AdjustDimensions(void) {
	chr_width  = (width  >> 1) + (width  & 0x1);
	chr_height = (height >> 1) + (height & 0x1);
}

void ColorCombinationFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	chr.SelectBufferParameters();
}

void ColorCombinationFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **  split_in_ptr_i = 0;
	uint8_t **        in_ptr_i = 0;
	fp16    **  split_in_ptr_f = 0;
	fp16    **        in_ptr_f = 0;
	void    **        in_ptr   = 0;
	uint8_t ***split_chr_ptr   = 0;
	uint8_t ***      chr_ptr   = 0;
	uint8_t **       out_ptr_i = 0;
	fp16    **       out_ptr_f = 0;
	void    **       out_ptr   = 0;

	if(line_idx == 0) {
		// This filter has NO shadow registers; select the default one
		SelectParameters();

		// Following hack ensures that sufficient chroma samples
		// are generated in case of odd image dimensions
		AdjustDimensions();
	}

	// Assign first and second dimensions of chroma pointers to
	// chroma input planes and chroma split input lines, respectively,
	// and allocate pointers to hold packed incoming chroma data
	split_chr_ptr = new uint8_t **[chr.GetNumPlanes() + 1];
	      chr_ptr = new uint8_t **[chr.GetNumPlanes() + 1];
	for(int pl = 0; pl <= chr.GetNumPlanes(); pl++) {
		split_chr_ptr[pl] = new uint8_t *[chr_kernel_size];
		      chr_ptr[pl] = new uint8_t *[chr_kernel_size];
		for(int ln = 0; ln < chr_kernel_size; ln++)
			chr_ptr[pl][ln] = new uint8_t[chr_width];
	}

	// Assign first dimension of luma pointers to luma split input lines
	// and allocate pointers to hold packed incoming luma data
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

	// Assign first dimension of pointers to output RGB lines
	// and prepare filter output buffer line pointers
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		out_ptr_i = new uint8_t *[out.GetNumPlanes() + 1];
		setOutputLines(out_ptr_i, out.GetNumPlanes());
		out_ptr = (void **)out_ptr_i;
	} else {
		out_ptr_f = new fp16    *[out.GetNumPlanes() + 1];
		setOutputLines(out_ptr_f, out.GetNumPlanes());
		out_ptr = (void **)out_ptr_f;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Prepare luma line pointers
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		getKernelLines(split_in_ptr_i, kernel_size, 0);
		PackInputLines(split_in_ptr_i, kernel_size, in_ptr_i);
	} else {
		getKernelLines(split_in_ptr_f, kernel_size, 0);
		PackInputLines(split_in_ptr_f, kernel_size, in_ptr_f);
	}

	// Prepare chroma line pointers then run
	getChromaLines (split_chr_ptr);
	PackChromaLines(split_chr_ptr, chr_kernel_size, chr_ptr);
	Run(in_ptr, (void ***)chr_ptr, out_ptr);

	// Update buffer fill levels and frame line indices
	UpdateBuffersSync();
	if(line_idx % 2)
		IncChrLineIdx();
	IncLineIdx();

	// At the end of the frame
	if (EndOfFrame()) {
		// Trigger end of frame interrupt
		EndOfFrameIrq();

		// Update the chroma buffer and line index if
		// the frame's height is odd
		if(height % 2) {
			UpdateChromaBufferSync();
			IncChrLineIdx();
		}
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_i;
	else
		delete[] split_in_ptr_f;

	delete[]        in_ptr;
	delete[] split_chr_ptr;
	delete[]       chr_ptr;
	delete[]       out_ptr;
}

void ColorCombinationFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t **  split_in_ptr_i = 0;
	uint8_t **        in_ptr_i = 0;
	fp16    **  split_in_ptr_f = 0;
	fp16    **        in_ptr_f = 0;
	void    **        in_ptr   = 0;
	uint8_t ***split_chr_ptr   = 0;
	uint8_t ***      chr_ptr   = 0;
	uint8_t **       out_ptr_i = 0;
	fp16    **       out_ptr_f = 0;
	void    **       out_ptr   = 0;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// This filter has NO shadow registers; select the default one
	if(line_idx == 0)
		SelectParameters();

	// Assign first and second dimensions of chroma pointers to
	// chroma input planes and chroma split input lines, respectively,
	// and allocate pointers to hold packed incoming chroma data
	split_chr_ptr = new uint8_t **[chr.GetNumPlanes() + 1];
	      chr_ptr = new uint8_t **[chr.GetNumPlanes() + 1];
	for(int pl = 0; pl <= chr.GetNumPlanes(); pl++) {
		split_chr_ptr[pl] = new uint8_t *[chr_kernel_size];
		      chr_ptr[pl] = new uint8_t *[chr_kernel_size];
		for(int ln = 0; ln < chr_kernel_size; ln++)
			chr_ptr[pl][ln] = new uint8_t[chr_width];
	}

	// Assign first dimension of luma pointers to luma split input lines
	// and allocate pointers to hold packed incoming luma data
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

	// Assign first dimension of pointers to output RGB lines
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		out_ptr_i = new uint8_t *[out.GetNumPlanes() + 1];
		out_ptr = (void **)out_ptr_i;
	} else {
		out_ptr_f = new fp16    *[out.GetNumPlanes() + 1];
		out_ptr = (void **)out_ptr_f;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRunCc(kernel_size, chr_kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Following hack ensures that sufficient chroma samples
		// are generated in case of odd image dimensions
		if(line_idx == 0)
			AdjustDimensions();

		// Prepare filter output buffer line pointers
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			setOutputLines(out_ptr_i, out.GetNumPlanes());
		else
			setOutputLines(out_ptr_f, out.GetNumPlanes());

		// Prepare luma line pointers
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_i, kernel_size, 0);
			PackInputLines(split_in_ptr_i, kernel_size, in_ptr_i);
		} else {
			getKernelLines(split_in_ptr_f, kernel_size, 0);
			PackInputLines(split_in_ptr_f, kernel_size, in_ptr_f);
		}

		// Prepare chroma line pointers then run
		getChromaLines (split_chr_ptr);
		PackChromaLines(split_chr_ptr, chr_kernel_size, chr_ptr);
		Run(in_ptr, (void ***)chr_ptr, out_ptr);

		// Update buffer fill levels and frame line indices
		UpdateBuffers();
		if(line_idx % 2)
			IncChrLineIdx();
		IncLineIdx();

		// At the end of the frame
		if (EndOfFrame()) {
			// Trigger end of frame interrupt
			EndOfFrameIrq();

			// Update the chroma buffer and line index if
			// the frame's height is odd
			if(height % 2) {
				UpdateChromaBuffer();
				IncChrLineIdx();
			}

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
		delete[] split_in_ptr_i;
	else
		delete[] split_in_ptr_f;

	delete[]        in_ptr;
	delete[] split_chr_ptr;
	delete[]       chr_ptr;
	delete[]       out_ptr;
}

void *ColorCombinationFilt::Run(void **in_ptr, void ***chr_ptr, void **out_ptr) {
	uint8_t **  luma_src_i = 0;
	fp16    **  luma_src_f = 0;
	uint8_t ***    chr_src = 0;
	int     *   chr_kernel = 0;
	uint8_t **       dst_i = 0;
	uint8_t **packed_dst_i = 0;
	fp16    **       dst_f = 0;
	fp16    **packed_dst_f = 0;

	// Allocating array of pointers to input luma lines
	// and setting luma pointers
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		luma_src_i = new uint8_t *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			luma_src_i[i] = (uint8_t *)in_ptr[i];
	} else {
		luma_src_f = new fp16 *[kernel_size];
		for (int i = 0; i < kernel_size; i++)
			luma_src_f[i] = (fp16 *)in_ptr[i];
	}

	// Allocating array of pointers to input chroma planes
	chr_src = new uint8_t **[chr.GetNumPlanes() + 1];

	// Allocating array of pointers to input chroma lines
	// and setting chroma pointers
	for(int pl = 0; pl <= chr.GetNumPlanes(); pl++) {
		chr_src[pl] = new uint8_t *[chr_kernel_size];
		for (int i = 0; i < chr_kernel_size; i++)
			chr_src[pl][i] = (uint8_t *)chr_ptr[pl][i];
	}
	
	// Allocating array of pointers to to output lines, setting output
	// pointers and allocating temporary packed planar output buffers
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_i = new uint8_t *[out.GetNumPlanes() + 1];
		packed_dst_i = new uint8_t *[out.GetNumPlanes() + 1];
		for (int pl = 0; pl <= out.GetNumPlanes(); pl++) {
			       dst_i[pl] = (uint8_t *)out_ptr[pl];
			packed_dst_i[pl] = new uint8_t[width];
		}
	} else {
		       dst_f = new fp16 *[out.GetNumPlanes() + 1];
		packed_dst_f = new fp16 *[out.GetNumPlanes() + 1];
		for (int pl = 0; pl <= out.GetNumPlanes(); pl++) {
			       dst_f[pl] = (fp16 *)out_ptr[pl];
			packed_dst_f[pl] = new fp16[width];
		}
	}

	// Output of vertical upscaling
	int **upscaled_v_kernel = new int*[chr.GetNumPlanes() + 1];
	for (int pl = 0; pl <= chr.GetNumPlanes(); pl++)
		upscaled_v_kernel[pl] = new int[chr_width];

	// Internal kernel on which the processing and padding are done
	chr_kernel = new int[chr_kernel_size];

	// A line of upscaled normalized chroma
	uint8_t **norm_chroma = new uint8_t*[chr.GetNumPlanes() + 1];
	for (int pl = 0; pl <= chr.GetNumPlanes(); pl++)
		norm_chroma[pl] = new uint8_t[chr_width << 1];

	// Holds luma combined with chroma
	uint16_t **combined = new uint16_t*[out.GetNumPlanes() + 1];
	for (int pl = 0; pl <= out.GetNumPlanes(); pl++)
		combined[pl] = new uint16_t[width];

	// Holds a line of 12bit luma values
	uint16_t *conv_luma = new uint16_t[width];

	// Determine which coefficients and data to be used with regard to the current phase
	char *used_kernel = (line_idx % 2) ? odd_coeffs : even_coeffs;
	int start_pos     = (line_idx % 2) ?     1      :      0     ;

	int uvk, uhk_p0, uhk_p1;
	uint16_t max_rgb;
	uint8_t alpha;
	int out_val;

	// Vertical upscaling
	int x, uhk_i;
	for(int pl = 0; pl <= chr.GetNumPlanes(); pl++) {
		x = uhk_i = 0;
		while (x < chr_width) {
			uvk = 0;
			for(int ln = start_pos, c_i = 0; c_i < chr_kernel_size - 1; ln++)
				uvk += chr_src[pl][ln][x] * used_kernel[c_i++];
			upscaled_v_kernel[pl][x] = uvk;
			x++;
		}

		// Horizontal upscaling
		x = 0;
		while (x < chr_width) {
			uhk_p0 = uhk_p1 = 0;
			buildChromaKernel(upscaled_v_kernel[pl], chr_kernel, x);
			for(int pix = 0; pix < chr_kernel_size - 1; pix++) {
				uhk_p0 += chr_kernel[pix + 0] * even_coeffs[pix];
				uhk_p1 += chr_kernel[pix + 1] *  odd_coeffs[pix];
			}

			// Normalize and round upscaled chroma
			uhk_p0 = CLIP(uhk_p0, 0xFF000);
			uhk_p1 = CLIP(uhk_p1, 0xFF000);
			norm_chroma[pl][uhk_i++] = DOWNCAR(uhk_p0, 12);
			norm_chroma[pl][uhk_i++] = DOWNCAR(uhk_p1, 12);
			x++;
		}
	}

	// Convert the current line of luma pixels to U12
	x = 0;
	while(x < width) {
		if(!force_luma_one) {
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				crt_conv_luma =  U8F_TO_U12F(luma_src_i[0][x]);
			} else {
				crt_conv_luma = FP16_TO_U12F(luma_src_f[0][x]);
			}
		}
		conv_luma[x] = crt_conv_luma;
		x++;
	}

	// Compute the RGB pixels
	for(int pl = 0; pl <= out.GetNumPlanes(); pl++) {
		x = 0;
		while(x < width) {
			uint64_t lck = (conv_luma[x] + epsilon) * norm_chroma[pl][x];
			lck = lck * k_rgb[pl].full;
			uint32_t scaled = (uint32_t)DOWNCAR(lck, 16);
			combined[pl][x] = CLIP(scaled, 0xFFF);
			x++;
		}
	}

	x = 0;
	while(x < width) {
		// Determine the maximum values among RGB planes
		max_rgb = MAX(combined[0][x], MAX(combined[1][x], combined[2][x]));
		int32_t tmp_alpha = (int32_t)floor(((max_rgb - conv_t1) * mul)/16.f + 0.5f);
		alpha = CLIP(tmp_alpha, 0xFF);

		// Chroma desaturation
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++) {
			uint32_t desaturated = (conv_luma[x] << 8) + alpha * (combined[pl][x] - conv_luma[x]);
			combined[pl][x] = (uint16_t)floor(desaturated/256.f + 0.5f);
			combined[pl][x] = CLIP(combined[pl][x], 0xFFF);
		}

		x++;
	}

	// Apply color adjustment and saturation
	int temp_storage;
	for(int pix = 0; pix < width; pix++) {
		for(int opl = 0; opl <= out.GetNumPlanes(); opl++) {
			temp_storage = 0;
			int ccm_i = opl;
			for (int pl = 0; pl <= out.GetNumPlanes(); pl++, ccm_i += 3) {
				temp_storage += combined[pl][pix] * (int16_t)ccm[ccm_i].full;
			}
			out_val = DOWNCAR(temp_storage, 10);
			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				int16_t out_val_u8 = U12F_TO_U8F(out_val);
				packed_dst_i[opl][pix] = (uint8_t)ClampWR((float)out_val_u8,   0.0f, (float)((1 << 8) - 1));
			} else {
				fp16 out_val_fp16 = U12F_TO_FP16(out_val);
				packed_dst_f[opl][pix] = ( fp16  )ClampWR((float)out_val_fp16, 0.0f, 1.0f);
			}
		}
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		for(int opl = 0; opl <= out.GetNumPlanes(); opl++)
			SplitOutputLine(packed_dst_i[opl], width, dst_i[opl]);
		delete[]        dst_i;
		delete[] packed_dst_i;
	} else {
		for(int opl = 0; opl <= out.GetNumPlanes(); opl++)
			SplitOutputLine(packed_dst_f[opl], width, dst_f[opl]);
		delete[]        dst_f;
		delete[] packed_dst_f;
	}

	for(int pl = 0; pl <= chr.GetNumPlanes(); pl++) {
		delete[] upscaled_v_kernel[pl];
		delete[] norm_chroma[pl];
		delete[] chr_src[pl];
	}
	delete[] upscaled_v_kernel;
	delete[] chr_kernel;
	delete[] norm_chroma;
	delete[] chr_src;

	for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
		delete[] combined[pl];
	delete[] combined;

	if(!force_luma_one)
		delete[] conv_luma;

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] luma_src_i;
	else
		delete[] luma_src_f;

	return out_ptr;
}

// Not used in this particular filter as it has his own version of Run
void *ColorCombinationFilt::Run(void **in_ptr, void *out_ptr) {
	return NULL;
}
