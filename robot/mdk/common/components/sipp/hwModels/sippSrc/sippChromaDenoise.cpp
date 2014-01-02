// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Chroma denoise filter
//
//
// -----------------------------------------------------------------------------

#include "sippChromaDenoise.h"

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


ChromaDenoiseFilt::ChromaDenoiseFilt(SippIrq *pObj, int id, int vk, int hk0, int hk1, int hk2, int rk, std::string name) :
	kernel_size (vk),
	ref_kernel_size (rk),
	ref("input", Read) {

		// pObj, name, id and kernel_lines are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);

		ref_max_pad = ref_kernel_size >> 1;

		cdn_horiz_pass_sizes[0] = hk0;
		cdn_horiz_pass_sizes[1] = hk1;
		cdn_horiz_pass_sizes[2] = hk2;
}

void ChromaDenoiseFilt::SetHorEnable(int val, int reg) {
	cdn_params[reg].hor_enable = val;
}

void ChromaDenoiseFilt::SetRefEnable(int val, int reg) {
	cdn_params[reg].ref_enable = val;
}

void ChromaDenoiseFilt::SetLimit(int val, int reg) {
	cdn_params[reg].limit = val;
}

void ChromaDenoiseFilt::SetForceWeightsHor(int val, int reg) {
	cdn_params[reg].force_weights_hor = val;
}

void ChromaDenoiseFilt::SetForceWeightsVer(int val, int reg) {
	cdn_params[reg].force_weights_ver = val;
}

void ChromaDenoiseFilt::SetThreePlaneMode(int val, int reg) {
	cdn_params[reg].three_plane_mode = val;
}

void ChromaDenoiseFilt::SetHorThreshold1(int val, int reg) {
	cdn_params[reg].hor_ths[0] = val;
}

void ChromaDenoiseFilt::SetHorThreshold2(int val, int reg) {
	cdn_params[reg].hor_ths[1] = val;
}

void ChromaDenoiseFilt::SetHorThreshold3(int val, int reg) {
	cdn_params[reg].hor_ths[2] = val;
}

void ChromaDenoiseFilt::SetVerThreshold1(int val, int reg) {
	cdn_params[reg].ver_ths[0] = val;
}

void ChromaDenoiseFilt::SetVerThreshold2(int val, int reg) {
	cdn_params[reg].ver_ths[1] = val;
}

void ChromaDenoiseFilt::SetVerThreshold3(int val, int reg) {
	cdn_params[reg].ver_ths[2] = val;
}

int ChromaDenoiseFilt::IncInputBufferFillLevel (void) {
	int ret = in.IncFillLevel();
	if(USE_REF)
		ref.IncFillLevel();
	TryRun();
	return ret;
}

bool ChromaDenoiseFilt::CanRunCdn(int kernel_size, int ref_kernel_size) {
	// At start of frame wait for buffer fill level to hit start line
	if (frame_count == 0 && line_idx == 0 && ( in.GetFillLevel() <  in.GetStartLevel())
		                 &&  ((USE_REF)   ?  (ref.GetFillLevel() < ref.GetStartLevel()) : true))
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
			min_fill = line_idx + max_pad + 1;                    // top
		else if (height - line_idx <= max_pad)				      
			min_fill = max_pad + height - line_idx;               // bottom
		else												      
			min_fill = kernel_size;                               // middle

		// Reference input padding
		if(USE_REF) {
			ref_max_pad = ref_kernel_size >> 1;
			if (line_idx < ref_max_pad)
				ref_min_fill = line_idx + ref_max_pad + 1;        // top
			else if (height - line_idx <= ref_max_pad)
				ref_min_fill = ref_max_pad + height - line_idx;   // bottom
			else
				ref_min_fill = ref_kernel_size;                   // middle
		}

		if (out.GetBuffLines() == 0) {
			can_run =     in.GetFillLevel() >=     min_fill &&
	        ((USE_REF) ? ref.GetFillLevel() >= ref_min_fill : true);
		} else {

			can_run =     in.GetFillLevel() >=     min_fill && out.GetFillLevel() < out.GetBuffLines() &&
			((USE_REF) ? ref.GetFillLevel() >= ref_min_fill : true);
		}
	}

	return can_run;
}

void ChromaDenoiseFilt::getReferenceLines(uint8_t **ref_ptr, int plane) {
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

void ChromaDenoiseFilt::PackReferenceLines(uint8_t **split_src, int kernel_size, uint8_t **packed_src) {
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

int ChromaDenoiseFilt::NextReferenceSlice(int offset) {
	return (ref_plane_start_slice + offset == g_last_slice + 1) ? g_first_slice - g_last_slice : 1;
}

void ChromaDenoiseFilt::ComputeReferenceStartSlice(int plane) {
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

void ChromaDenoiseFilt::PadAtTop(uint8_t **ref_ptr, int pad_lines, int plane) {
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

void ChromaDenoiseFilt::PadAtBottom(uint8_t **ref_ptr, int pad_lines, int plane) {
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

void ChromaDenoiseFilt::MiddleExecutionV(uint8_t **ref_ptr, int plane) {
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

void ChromaDenoiseFilt::UpdateBuffers(void) {
	if(out.GetBuffLines() == 0)
		UpdateOutputBufferNoWrap();
	else
		UpdateOutputBuffer();
	if(in.GetBuffLines() == 0) {
		UpdateInputBufferNoWrap();
		if(USE_REF)
			UpdateReferenceBufferNoWrap();
	} else {
		UpdateInputBuffer();
		if(USE_REF)
			UpdateReferenceBuffer();
	}
}

void ChromaDenoiseFilt::UpdateBuffersSync(void) {
	UpdateOutputBufferSync();
	UpdateInputBufferSync();
	if(USE_REF)
		UpdateReferenceBufferSync();
}

void ChromaDenoiseFilt::UpdateReferenceBuffer(void) {
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

void ChromaDenoiseFilt::UpdateReferenceBufferSync(void) {
	if (line_idx == (height - 1))
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= ref_max_pad; i++)
			ref.IncBufferIdx();
	else if (ref_max_pad - line_idx <= 0)
		// Not padding at top - decrement input buffer fill level, increment 
		// input buffer line index and set interrupt status
		ref.IncBufferIdx();
}

void ChromaDenoiseFilt::UpdateReferenceBufferNoWrap(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= ref_max_pad; i++)
			ref.IncBufferIdxNoWrap(height);
	} else if (ref_max_pad - line_idx <= 0) {
		// Not padding at top - increment input buffer line index
		ref.IncBufferIdxNoWrap(height);
	}
}

void ChromaDenoiseFilt::buildRefKernel(uint8_t **src, uint8_t *kernel, int x) {
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

void ChromaDenoiseFilt::PadAtLeft(uint8_t **src, uint8_t *kernel, int pad_pixels, int crt_pixel) {
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

void ChromaDenoiseFilt::PadAtRight(uint8_t **src, uint8_t *kernel, int pad_pixels, int crt_pixel) {
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

void ChromaDenoiseFilt::MiddleExecutionH(uint8_t **src, uint8_t *kernel, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < ref_kernel_size; kv++)
		for (kh = 0; kh < ref_kernel_size; kh++)
			kernel[kv*ref_kernel_size + kh] = src[kv][crt_pixel + kh - ref_max_pad];
}

uint8_t ChromaDenoiseFilt::ControlDenoising(uint8_t noisy, uint8_t filtered) {
	int delta = filtered - noisy;

	delta = (delta >= -limit && delta <= limit) ? delta : (delta > limit) ? limit : -limit;

	return (uint8_t)(noisy + delta);
}

void ChromaDenoiseFilt::AssertThresholds(void) {
	if(GetThreePlaneMode() == ENABLED) {
		int all_nz = 1;
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			all_nz *= hor_ths[pl] * ver_ths[pl];
		}
		if(!all_nz) {
			std::cerr << "All thresholds must be non zero in three plane mode." << std::endl;
			abort();
		}
	} else {
		if((hor_ths[0] == hor_ths[1] && hor_ths[1] == 0) || (ver_ths[0] == ver_ths[1] && ver_ths[1] == 0)) {
			std::cerr << "Invalid configuration. At least one of the horizontal and one of the vertical thresholds must be non zero." << std::endl;
			abort();
		}
	}
}

void ChromaDenoiseFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	if(USE_REF)
		ref.SelectBufferParameters();
	hor_enable        = cdn_params[reg].hor_enable;
	ref_enable        = cdn_params[reg].ref_enable;
	limit             = cdn_params[reg].limit;
	force_weights_hor = cdn_params[reg].force_weights_hor;
	force_weights_ver = cdn_params[reg].force_weights_ver;
	three_plane_mode  = cdn_params[reg].three_plane_mode;
	for(int ths = 0; ths < 3; ths++) {
		hor_ths[ths] = cdn_params[reg].hor_ths[ths];
		ver_ths[ths] = cdn_params[reg].ver_ths[ths];
	}
}

void ChromaDenoiseFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t ***split_in_ptr = 0;
	uint8_t ***      in_ptr = 0;
	uint8_t **split_ref_ptr = 0;
	uint8_t **      ref_ptr = 0;
	uint8_t **      out_ptr = 0;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	split_in_ptr = new uint8_t **[in.GetNumPlanes() + 1];
	      in_ptr = new uint8_t **[in.GetNumPlanes() + 1];
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		split_in_ptr[pl] = new uint8_t *[kernel_size];
		      in_ptr[pl] = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr[pl][ln] = new uint8_t[width];
	}

	// Assign first dimension of pointers to reference split input lines
	// and allocate pointers to hold packed incoming reference data
	if(USE_REF) {
		split_ref_ptr = new uint8_t *[kernel_size];
		      ref_ptr = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			ref_ptr[ln] = new uint8_t[width];
	}

	// Assign first dimension of pointers to output planes
	// and prepare filter output buffer line pointers
	out_ptr = new uint8_t *[out.GetNumPlanes() + 1];
	setOutputLines(out_ptr, out.GetNumPlanes());

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Check for threshold's validity
	if(line_idx == 0)
		AssertThresholds();

	// Filter all the planes simultaneously
	if(GetThreePlaneMode() == ENABLED) {
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			getKernelLines(split_in_ptr[pl], kernel_size, pl);
			PackInputLines(split_in_ptr[pl], kernel_size, in_ptr[pl]);
		}
		Run((void ***)in_ptr, (void **)out_ptr);
	// Filter a line from each plane
	} else {
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			getKernelLines(split_in_ptr[pl], kernel_size, pl);
			PackInputLines(split_in_ptr[pl], kernel_size, in_ptr[pl]);
			if(USE_REF) {
				getReferenceLines (split_ref_ptr, pl);
				PackReferenceLines(split_ref_ptr, kernel_size, ref_ptr);
			}
			Run((void **)in_ptr[pl], (void **)ref_ptr, (void *)out_ptr[pl]);
		}
	}
	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		delete[] split_in_ptr[pl];
		delete[]       in_ptr[pl];
	}
	delete[] split_in_ptr;
	delete[]       in_ptr;
	if(USE_REF) {
		delete[] split_ref_ptr;
		delete[]       ref_ptr;
	}
	delete[] out_ptr;
}

void ChromaDenoiseFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t ***split_in_ptr = 0;
	uint8_t ***      in_ptr = 0;
	uint8_t **split_ref_ptr = 0;
	uint8_t **      ref_ptr = 0;
	uint8_t **      out_ptr = 0;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	split_in_ptr = new uint8_t **[in.GetNumPlanes() + 1];
	      in_ptr = new uint8_t **[in.GetNumPlanes() + 1];
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		split_in_ptr[pl] = new uint8_t *[kernel_size];
		      in_ptr[pl] = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			in_ptr[pl][ln] = new uint8_t[width];
	}

	// Assign first dimension of pointers to reference split input lines
	// and allocate pointers to hold packed incoming reference data
	if(USE_REF) {
		split_ref_ptr = new uint8_t *[kernel_size];
		      ref_ptr = new uint8_t *[kernel_size];
		for(int ln = 0; ln < kernel_size; ln++)
			ref_ptr[ln] = new uint8_t[width];
	}

	// Assign first dimension of pointers to output planes
	out_ptr = new uint8_t *[out.GetNumPlanes() + 1];

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRunCdn(kernel_size, ref_kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Check for threshold's validity
		if(line_idx == 0)
			AssertThresholds();

		// Prepare filter output buffer line pointers
		setOutputLines(out_ptr, out.GetNumPlanes());

		// Filter all the planes simultaneously
		if(GetThreePlaneMode() == ENABLED) {
			for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
				// Prepare filter buffer line pointers then run
				getKernelLines(split_in_ptr[pl], kernel_size, pl);
				PackInputLines(split_in_ptr[pl], kernel_size, in_ptr[pl]);
			}
			Run((void ***)in_ptr, (void **)out_ptr);
		// Filter a line from each plane
		} else {
			for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
				// Prepare filter buffer line pointers then run
				getKernelLines(split_in_ptr[pl], kernel_size, pl);
				PackInputLines(split_in_ptr[pl], kernel_size, in_ptr[pl]);
				if(USE_REF) {
					getReferenceLines (split_ref_ptr, pl);
					PackReferenceLines(split_ref_ptr, kernel_size, ref_ptr);
				}
				Run((void **)in_ptr[pl], (void **)ref_ptr, (void *)out_ptr[pl]);
			}
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
	
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		delete[] split_in_ptr[pl];
		delete[]       in_ptr[pl];
	}
	delete[] split_in_ptr;
	delete[]       in_ptr;
	if(USE_REF) {
		delete[] split_ref_ptr;
		delete[]       ref_ptr;
	}
	delete[] out_ptr;
}

void ChromaDenoiseFilt::Run(void ***in_ptr, void **out_ptr) {
	uint8_t ***      src = 0;
	uint8_t **       dst = 0;
	uint8_t **packed_dst = 0;
	uint8_t **denoise_1D = 0;
	uint8_t **   temp_1D = 0;
	uint8_t **   kernels = 0;

	// Allocating array of pointers to input chroma planes
	src = new uint8_t **[in.GetNumPlanes() + 1];

	// Allocating array of pointers to input chroma lines
	// and setting chroma pointers
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		src[pl] = new uint8_t *[kernel_size];
		for (int ln = 0; ln < kernel_size; ln++)
			src[pl][ln] = (uint8_t *)in_ptr[pl][ln];
	}

	// Planar output of vertical/horizontal denoise filter passes
	denoise_1D = new uint8_t *[in.GetNumPlanes() + 1];
	   temp_1D = new uint8_t *[in.GetNumPlanes() + 1];
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		denoise_1D[pl] = new uint8_t[width];
		   temp_1D[pl] = new uint8_t[width];
	}

	// Allocating array of pointers to to output lines, setting output
	// pointers and allocating temporary packed planar output buffers
	       dst = new uint8_t *[out.GetNumPlanes() + 1];
	packed_dst = new uint8_t *[out.GetNumPlanes() + 1];
	for(int pl = 0; pl <= out.GetNumPlanes(); pl++) {
		       dst[pl] = (uint8_t *)out_ptr[pl];
		packed_dst[pl] = new uint8_t[width];
	}

	int x = 0;
	
	uint8_t wt;
	uint8_t wt_tmp;
	int16_t diff;
	uint8_t *shifted_in = new uint8_t [in.GetNumPlanes() + 1];
	uint32_t *acc       = new uint32_t[in.GetNumPlanes() + 1];

	// Vertical pass
	while(x < width) {
		// Reset the final weight and accumulators
		wt = 0;
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
			acc[pl] = 0;

		// Determine difference between centre and neighbours for each plane
		for(int ln = 0; ln < kernel_size; ln++) {
			wt_tmp = 1;
			for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
				shifted_in[pl] = src[pl][ln][x];
				if(!force_weights_ver) {
					diff = abs(src[pl][V_KERNEL_CENTRE][x] - shifted_in[pl]);
					wt_tmp &= (diff < ver_ths[pl]);
				}
			}
			// Accumulate the unique set of weigths
			wt += wt_tmp;

			// Accumulate the pixels
			for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
				acc[pl] += wt_tmp * shifted_in[pl];
		}

		// Average the vertical pass results
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
			denoise_1D[pl][x] = (uint8_t)((acc[pl] + (wt >> 1)) / wt);
		x++;
	}

	// Horizontal passes
	for(int pass = 0; pass < sizeof(cdn_horiz_pass_sizes)/sizeof(int); pass++) {
		if(ENABLED_PASS(pass)) {
			// Preserve result in order not to alter the kernels
			for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
				memcpy(temp_1D[pl], denoise_1D[pl], sizeof(**denoise_1D)*width);
			x = 0;

			// 1D kernels for planar horizontal denoise filter pass
			kernels = new uint8_t *[in.GetNumPlanes() + 1];
			for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
				kernels[pl] = new uint8_t[cdn_horiz_pass_sizes[pass]];

			while(x < width) {
				for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
					// Horizontal padding of vertical denoise filter output
					// or previous horizontal denoise filter output
					buildKernel(temp_1D[pl], kernels[pl], cdn_horiz_pass_sizes[pass], x);

					// Reset accumulators
					acc[pl] = 0;
				}

				// Reset the final weight
				wt = 0;

				// Determine difference between centre and neighbours for each plane
				for(int pix = 0; pix < cdn_horiz_pass_sizes[pass]; pix++) {
					wt_tmp = 1;
					for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
						shifted_in[pl] = kernels[pl][pix];
						if(!force_weights_hor) {
							diff = abs(kernels[pl][H_KERNEL_CENTRE(pass)] - shifted_in[pl]);
							wt_tmp &= (diff < hor_ths[pl]);
						}
					}
					// Accumulate the unique set of weigths
					wt += wt_tmp;

					// Accumulate the pixels
					for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
						acc[pl] += wt_tmp * shifted_in[pl];
				}
				
				// Average the current horizontal pass results
				for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
					denoise_1D[pl][x] = (uint8_t)((acc[pl] + (wt >> 1)) / wt);
				x++;
			}
			for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
				delete[] kernels[pl];
			delete[] kernels;
		}
	}
	// Don't allow the filter to change the pixel value by more than "limit"
	x = 0;
	while(x < width) {
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
			packed_dst[pl][x] = ControlDenoising(src[pl][V_KERNEL_CENTRE][x], denoise_1D[pl][x]);
		x++;
	}

	for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
		SplitOutputLine(packed_dst[pl], width, dst[pl]);

	delete[] shifted_in; delete[] acc;
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		delete[] denoise_1D[pl];
		delete[] temp_1D[pl];
		delete[] src[pl];
	}
	delete[] denoise_1D;
	delete[]    temp_1D;
	delete[]        src;
	delete[]        dst;
	delete[] packed_dst;
}

void ChromaDenoiseFilt::Run(void **in_ptr_orig, void **in_ptr_ref, void *out_ptr) {
	uint8_t **      src = 0;
	uint8_t **      ref = 0;
	uint8_t *       dst = 0;
	uint8_t *packed_dst = 0;
	uint8_t *denoise_1D = 0;
	uint8_t *   temp_1D = 0;
	uint8_t *    kernel = 0;
	uint8_t *ref_kernel = 0;

	// Allocating array of pointers to original input lines
	// and setting original input pointers
	src = new uint8_t *[kernel_size];
	for (int ln = 0; ln < kernel_size; ln++) {
		src[ln] = (uint8_t *)in_ptr_orig[ln];
	}

	// Allocating array of pointers to reference input lines and setting
	// reference input pointers if there's a reference stream used
	if(USE_REF) {
		ref = new uint8_t *[ref_kernel_size];
		for (int ln = 0; ln < ref_kernel_size; ln++) {
			ref[ln] = (uint8_t *)in_ptr_ref[ln];
		}
	}

	// Output of vertical/horizontal denoise filter passes
	denoise_1D = new uint8_t[width];
	temp_1D    = new uint8_t[width];

	// Setting output pointer and allocating temporary packed output buffer
	       dst = (uint8_t *)out_ptr;
	packed_dst = new uint8_t[width];

	int x = 0;

	int diff;
	int shifted_in;
	int shifted_ref;
	int wt;
	int wt_tmp;
	int acc;

	// Vertical pass
	while(x < width) {
		// Reset weight and accumulator
		wt = acc = 0;

		// Determine difference between centre and neighbours, accumulate weigths and pixels
		for(int ln = 0; ln < kernel_size; ln++) {
			shifted_in  = src[ln][x];
			if(force_weights_ver) {
				wt_tmp = 1;
			} else {
				if(USE_REF) {
					shifted_ref = ref[ln][x];
					diff = abs(ref[V_KERNEL_CENTRE][x] - shifted_ref);
				} else {
					diff = abs(src[V_KERNEL_CENTRE][x] - shifted_in );
				}
				wt_tmp = (diff < ver_ths[0]) + (diff < ver_ths[1]);
			}
			wt  += wt_tmp;
			acc += wt_tmp * shifted_in;
		}

		// Average the vertical pass results
		denoise_1D[x] = (uint8_t)((acc + (wt >> 1)) / wt);
		x++;
	}

	// Horizontal passes
	for(int pass = 0; pass < sizeof(cdn_horiz_pass_sizes)/sizeof(int); pass++) {
		if(ENABLED_PASS(pass)) {
			// Preserve result in order not to alter the kernels
			memcpy(temp_1D, denoise_1D, sizeof(*denoise_1D)*width);
			x = 0;

			// 1D kernel for horizontal denoise filter pass
			kernel = new uint8_t[cdn_horiz_pass_sizes[pass]];
			if(USE_REF_HOR)
				ref_kernel = new uint8_t[cdn_horiz_pass_sizes[pass]];

			while(x < width) {
				// Horizontal padding of vertical denoise filter output
				// or previous horizontal denoise filter output
				buildKernel(temp_1D, kernel, cdn_horiz_pass_sizes[pass], x);
				if(USE_REF_HOR)
					buildKernel(ref[V_KERNEL_CENTRE], ref_kernel, cdn_horiz_pass_sizes[pass], x);

				// Reset weight and accumulator
				wt = acc = 0;

				// Determine difference between centre and neighbours, accumulate weigths and pixels
				for(int pix = 0; pix < cdn_horiz_pass_sizes[pass]; pix++) {
					shifted_in = kernel[pix];
					if(force_weights_hor) {
						wt_tmp = 1;
					} else {
						if(USE_REF_HOR) {
							shifted_ref = ref_kernel[pix];
							diff = abs(ref_kernel[H_KERNEL_CENTRE(pass)] - shifted_ref);
						} else {
							diff = abs(kernel[H_KERNEL_CENTRE(pass)] - shifted_in);
						}
						wt_tmp = (diff < hor_ths[0]) + (diff < hor_ths[1]);
					}
					wt  += wt_tmp;
					acc += wt_tmp * shifted_in;
				}

				// Average the current horizontal pass results
				denoise_1D[x] = (uint8_t)((acc + (wt >> 1)) / wt);
				x++;
			}
			delete[] kernel;
			if(USE_REF_HOR)
				delete[] ref_kernel;
		}
	}
	// Don't allow the filter to change the pixel value by more than "limit"
	x = 0;
	while(x < width) {
		packed_dst[x] = ControlDenoising(src[V_KERNEL_CENTRE][x], denoise_1D[x]);
		x++;
	}

	SplitOutputLine(packed_dst, width, dst);

	delete[] src;
	if(USE_REF)
		delete[] ref;
	delete[] denoise_1D;
	delete[] temp_1D;
}

// Not used in this particular filter as it has his own versions of Run
void *ChromaDenoiseFilt::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}