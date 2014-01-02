// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW base filter model
// -----------------------------------------------------------------------------

#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "sippBase.h"

// Define static members
int SippBaseFilt::g_slice_size = SLICE_SIZE;
int SippBaseFilt::g_first_slice;
int SippBaseFilt::g_last_slice;

int SippBaseFilt::IncInputBufferFillLevel (void) {
	int ret = in.IncFillLevel();
	if (0)
		std::cout << "IncInputBufferFillLevel(): TryRun()" << std::endl;
	TryRun();
	return ret;
}

int SippBaseFilt::DecOutputBufferFillLevel (void) {
	int ret = out.DecFillLevel();
	if (0)
		std::cout << "DecOutputBufferFillLevel(): TryRun()" << std::endl;
	TryRun();
	return ret;
}

// Example implementation of virtual TryRun() function, suitable for use with 
// MemCpyFilt (or any other single line filter function). Re-implement as 
// necessary...
void SippBaseFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	fp16 **in_ptr;
	fp16 *out_ptr;

	// Assign first dimension of pointer to input lines
	in_ptr = new fp16 *[kernel_lines];

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_lines)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			getKernelLines(in_ptr, kernel_lines, pl);
			setOutputLine(&out_ptr, pl);
			Run((void **)in_ptr, (void *)out_ptr);
		}
		// Update buffer fill levels and input frame line index 
		UpdateBuffers();
		IncLineIdx();
	}
	cs.Leave();
	delete[] in_ptr;
}

void SippBaseFilt::SelectParameters(void) {
	width    = base_params[reg].width;
	width_o  = base_params[reg].width_o;
	height   = base_params[reg].height;
	height_o = base_params[reg].height_o;
	max_pad  = base_params[reg].max_pad;
	in.SelectBufferParameters();
	out.SelectBufferParameters();
}

bool SippBaseFilt::CanRun(int kernel_size) {
	// At start of frame wait for buffer fill level to hit start line
	if (frame_count == 0 && line_idx == 0 && (in.GetFillLevel() < in.GetStartLevel()))
		return false;

	bool can_run;
	if (in.GetBuffLines() == 0) {
		if (out.GetBuffLines() == 0) {
			can_run = true;
		} else {
			if (0 && !(out.GetFillLevel() < out.GetBuffLines()))
				std::cout << "CanRun(): output buffer full (" << out.GetFillLevel() << ")" << std::endl;

			can_run = out.GetFillLevel() < out.GetBuffLines();
		}
	} else {
		max_pad = kernel_size >> 1;
		if (line_idx < max_pad)
			min_fill = line_idx + max_pad + 1;             // top
		else if (height - line_idx <= max_pad)
			min_fill = max_pad + height - line_idx;        // bottom
		else
			min_fill = kernel_size;                        // middle

		if (0 && !(in.GetFillLevel() >= min_fill))
			std::cout << "CanRun(): insufficient input buffer fill level (" << in.GetFillLevel() << ")" << std::endl;

		if (out.GetBuffLines() == 0) {
			can_run = in.GetFillLevel() >= min_fill;
		} else {
			if (0 && !(out.GetFillLevel() < out.GetBuffLines()))
				std::cout << "CanRun(): output buffer full (" << out.GetFillLevel() << ")" << std::endl;

			can_run = in.GetFillLevel() >= min_fill && out.GetFillLevel() < out.GetBuffLines();
		}
	}

	return can_run;
}

bool SippBaseFilt::CanRunNoPad(int kernel_size) {
	// At start of frame wait for buffer fill level to hit start line
	if (frame_count == 0 && line_idx == 0 && (in.GetFillLevel() < in.GetStartLevel()))
		return false;

	bool can_run;
	if (in.GetBuffLines() == 0) {
		if (out.GetBuffLines() == 0) {
			can_run = true;
		} else {
			can_run = out.GetFillLevel() < out.GetBuffLines();
		}
	} else {
		// No padding performed
		min_fill = kernel_size;
		max_pad  = 0;

		if (out.GetBuffLines() == 0) {
			can_run = in.GetFillLevel() >= min_fill;
		} else {
			can_run = in.GetFillLevel() >= min_fill && out.GetFillLevel() < out.GetBuffLines();
		}
	}

	return can_run;
}

int SippBaseFilt::IncLineIdxNoPad(int kernel_size) {
	if (++line_idx >= (height - (kernel_size - 1))) {
		line_idx = 0;
		frame_count++;
		if (Verbose())
			std::cout << name << " processed " << frame_count << " frames" << std::endl;
	}
	return line_idx;
}

int SippBaseFilt::IncLineIdx(void) {
	if (++line_idx >= height) {
		line_idx = 0;
		frame_count++;
		if (Verbose())
			std::cout << name << " processed " << frame_count << " frames" << std::endl;
	}
	return line_idx;
}

int SippBaseFilt::IncOutputLineIdx(void) {
	if (++line_idx_o >= height_o) {
		line_idx_o = 0;
	}
	return line_idx_o;
}

int SippBaseFilt::IncOutputLineIdx(int plane) {
	if(plane == in.GetNumPlanes()) {
		if (++line_idx_o >= height_o)
			line_idx_o = 0;
	}
	return line_idx_o;
}

void SippBaseFilt::UpdateBuffers(void) {
	if(out.GetBuffLines() == 0)
		UpdateOutputBufferNoWrap();
	else
		UpdateOutputBuffer();
	if(in.GetBuffLines() == 0)
		UpdateInputBufferNoWrap();
	else
		UpdateInputBuffer();
}

void SippBaseFilt::UpdateBuffersNoPad(int kernel_size) {
	if(out.GetBuffLines() == 0)
		UpdateOutputBufferNoPadNoWrap(kernel_size);
	else
		UpdateOutputBuffer();
	if(in.GetBuffLines() == 0)
		UpdateInputBufferNoPadNoWrap(kernel_size);
	else
		UpdateInputBufferNoPad(kernel_size);
}

void SippBaseFilt::UpdateBuffersSync(void) {
	UpdateOutputBufferSync();
	UpdateInputBufferSync();
}

void SippBaseFilt::UpdateBuffersSyncNoPad(int kernel_size) {
	UpdateOutputBufferSync();
	UpdateInputBufferSyncNoPad(kernel_size);
}

void SippBaseFilt::UpdateOutputBuffer(void) {
	// Increment output buffer fill level, buffer line index and set
	// interrupt status
	out.IncFillLevel();
	out.IncBufferIdx();
	OutputBufferLevelIncIrq();
}

void SippBaseFilt::UpdateOutputBufferSync(void) {
	// Increment output buffer line index and set interrupt status
	out.IncBufferIdx();
	OutputBufferLevelIncIrq();
}

void SippBaseFilt::UpdateOutputBufferNoWrap(void) {
	// Increment output buffer line index
	out.IncBufferIdxNoWrap(height);
	// At the end of the frame
	if(out.GetBufferIdx() == 0)
		// Set interrupt status
		OutputBufferLevelIncIrq();
}

void SippBaseFilt::UpdateOutputBufferNoPadNoWrap(int kernel_size) {
	// Increment output buffer line index
	out.IncBufferIdxNoWrap(height - (kernel_size - 1));
	// At the end of the cropped frame
	if(out.GetBufferIdx() == 0)
		// Set interrupt status
		OutputBufferLevelIncIrq();
}

void SippBaseFilt::UpdateInputBuffer(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= max_pad; i++) {
			in.DecFillLevel();
			in.IncBufferIdx();
		}
		InputBufferLevelDecIrq();
	} else if (max_pad - line_idx <= 0) {
		// Not padding at top - decrement input buffer fill level, increment 
		// input buffer line index and set interrupt status
		in.DecFillLevel();
		in.IncBufferIdx();
		InputBufferLevelDecIrq();
	}
}

void SippBaseFilt::UpdateInputBufferNoPad(int kernel_size) {
	if (line_idx == height - kernel_size) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i < kernel_size; i++) {
			in.DecFillLevel();
			in.IncBufferIdx();
		}
		InputBufferLevelDecIrq();
	} else {
		// Not padding at top - decrement input buffer fill level, increment 
		// input buffer line index and set interrupt status
		in.DecFillLevel();
		in.IncBufferIdx();
		InputBufferLevelDecIrq();
	}
}

void SippBaseFilt::UpdateInputBufferSync(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= max_pad; i++)
			in.IncBufferIdx();
		InputBufferLevelDecIrq();
	} else if (max_pad - line_idx <= 0) {
		// Not padding at top - increment input buffer line index and set interrupt status
		in.IncBufferIdx();
		InputBufferLevelDecIrq();
	}
}

void SippBaseFilt::UpdateInputBufferSyncNoPad(int kernel_size) {
	if (line_idx == height - kernel_size) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i < kernel_size; i++)
			in.IncBufferIdx();
		InputBufferLevelDecIrq();
	} else {
		// Not padding at top - increment input buffer line index and set interrupt status
		in.IncBufferIdx();
		InputBufferLevelDecIrq();
	}
}

void SippBaseFilt::UpdateInputBufferNoWrap(void) {
	if (line_idx == (height - 1)) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i <= max_pad; i++)
			in.IncBufferIdxNoWrap(height);
		InputBufferLevelDecIrq();
	} else if (max_pad - line_idx <= 0) {
		// Not padding at top - increment input buffer line index
		in.IncBufferIdxNoWrap(height);
	}
}

void SippBaseFilt::UpdateInputBufferNoPadNoWrap(int kernel_size) {
	if (line_idx == height - kernel_size) {
		// Last line - flush the lines at the bottom of the buffer out...
		for (int i = 0; i < kernel_size; i++)
			in.IncBufferIdxNoWrap(height);
		InputBufferLevelDecIrq();
	} else {
		// Not padding at top - increment input buffer line index
		in.IncBufferIdxNoWrap(height);
	}
}

void SippBaseFilt::getKernelLines(uint8_t **in_ptr, int kernel_size, int plane) {
	int pad_lines;

	// Determine in what slice does the current plane start
	ComputeInputStartSlice(plane);

	if ((pad_lines = max_pad - line_idx) > 0) {
		// Padding at top
		PadAtTop(in_ptr, kernel_size, pad_lines, plane);

	} else if ((pad_lines = line_idx + max_pad - (height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(in_ptr, kernel_size, pad_lines, plane);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(in_ptr, kernel_size, plane);
	}
}

void SippBaseFilt::PadAtTop(uint8_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address =  (uint8_t *)in.buffer + sstride;

	for ( ; kv < pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx() 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv - pad_lines) 
			+ pstride * plane;
}

void SippBaseFilt::PadAtBottom(uint8_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address =  (uint8_t *)in.buffer + sstride;

	for ( ; kv < kernel_size - pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = in_ptr[kv - 1];
}

void SippBaseFilt::MiddleExecutionV(uint8_t **in_ptr, int kernel_size, int plane) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int kv = 0;
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address =  (uint8_t *)in.buffer + sstride;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;
}

void SippBaseFilt::getKernelLines(uint16_t **in_ptr, int kernel_size, int plane) {
	int pad_lines;

	// Determine in what slice does the current plane start
	ComputeInputStartSlice(plane);

	if ((pad_lines = max_pad - line_idx) > 0) {
		// Padding at top
		PadAtTop(in_ptr, kernel_size, pad_lines, plane);

	} else if ((pad_lines = line_idx + max_pad - (height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(in_ptr, kernel_size, pad_lines, plane);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(in_ptr, kernel_size, plane);
	}
}

void SippBaseFilt::PadAtTop(uint16_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint16_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint16_t *)in.buffer + sstride;

	for ( ; kv < pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx() 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv - pad_lines) 
			+ pstride * plane;
}

void SippBaseFilt::PadAtBottom(uint16_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint16_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint16_t *)in.buffer + sstride;

	for ( ; kv < kernel_size - pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = in_ptr[kv - 1];
}

void SippBaseFilt::MiddleExecutionV(uint16_t **in_ptr, int kernel_size, int plane) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int kv = 0;
	int lstride, pstride, sstride;
	uint16_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint16_t *)in.buffer + sstride;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;
}

void SippBaseFilt::getKernelLines(uint32_t **in_ptr, int kernel_size, int plane) {
	int pad_lines;

	// Determine in what slice does the current plane start
	ComputeInputStartSlice(plane);

	if ((pad_lines = max_pad - line_idx) > 0) {
		// Padding at top
		PadAtTop(in_ptr, kernel_size, pad_lines, plane);

	} else if ((pad_lines = line_idx + max_pad - (height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(in_ptr, kernel_size, pad_lines, plane);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(in_ptr, kernel_size, plane);
	}
}

void SippBaseFilt::PadAtTop(uint32_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint32_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint32_t *)in.buffer + sstride;

	for ( ; kv < pad_lines; kv++)
		in_ptr[kv] = start_slice_address  
			+ lstride * in.GetBufferIdx() 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv - pad_lines) 
			+ pstride * plane;
}

void SippBaseFilt::PadAtBottom(uint32_t **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	uint32_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint32_t *)in.buffer + sstride;

	for ( ; kv < kernel_size - pad_lines; kv++)
		in_ptr[kv] = start_slice_address  
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = in_ptr[kv - 1];
}

void SippBaseFilt::MiddleExecutionV(uint32_t **in_ptr, int kernel_size, int plane) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int kv = 0;
	int lstride, pstride, sstride;
	uint32_t *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address = (uint32_t *)in.buffer + sstride;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;
}

void SippBaseFilt::getKernelLines(fp16 **in_ptr, int kernel_size, int plane) {
	int pad_lines;

	// Determine in what slice does the current plane start
	ComputeInputStartSlice(plane);

	if ((pad_lines = max_pad - line_idx) > 0) {
		// Padding at top
		PadAtTop(in_ptr, kernel_size, pad_lines, plane);

	} else if ((pad_lines = line_idx + max_pad - (height - 1)) > 0) {
		// Padding at bottom
		PadAtBottom(in_ptr, kernel_size, pad_lines, plane);

	} else {
		pad_lines = 0;
		// Middle
		MiddleExecutionV(in_ptr, kernel_size, plane);
	}
}

void SippBaseFilt::PadAtTop(fp16 **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the first line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	fp16 *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address   =   (fp16 *)in.buffer + sstride;

	for ( ; kv < pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx() 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv - pad_lines) 
			+ pstride * plane;
}

void SippBaseFilt::PadAtBottom(fp16 **in_ptr, int kernel_size, int pad_lines, int plane) {
	// Replicate the last line by a number of times based on the kernel
	// size and the line that contains the current kernel centre (line_idx)
	int kv = 0;
	int lstride, pstride, sstride;
	fp16 *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address   =   (fp16 *)in.buffer + sstride;

	for ( ; kv < kernel_size - pad_lines; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = in_ptr[kv - 1];
}

void SippBaseFilt::MiddleExecutionV(fp16 **in_ptr, int kernel_size, int plane) {
	// No padding needed since we have the necessary lines for the
	// respective kernel size in order to continue the execution
	int kv = 0;
	int lstride, pstride, sstride;
	fp16 *start_slice_address;

	lstride  =  in.GetLineStride () / in.GetFormat();
	pstride  = (in.GetPlaneStride() / in.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size        / in.GetFormat()) * input_plane_start_slice;
	start_slice_address   =   (fp16 *)in.buffer + sstride;

	for ( ; kv < kernel_size; kv++)
		in_ptr[kv] = start_slice_address 
			+ lstride * in.GetBufferIdx(kv) 
			+ pstride * plane;
}

void SippBaseFilt::buildKernel (uint8_t **src, uint8_t *kernel, int kernel_size, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(uint8_t **src, uint8_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][0];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::PadAtRight(uint8_t **src, uint8_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size - pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = kernel_size - pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = kernel[kv*kernel_size + kh - 1];
}

void SippBaseFilt::MiddleExecutionH(uint8_t **src, uint8_t *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::buildKernel (uint8_t **src, uint16_t *kernel, int kernel_size, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(uint8_t **src, uint16_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;
	
	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = U8F_TO_U12F(src[kv][0]);

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = U8F_TO_U12F(src[kv][crt_pixel + kh - max_pad]);
}

void SippBaseFilt::PadAtRight(uint8_t **src, uint16_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size - pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = U8F_TO_U12F(src[kv][crt_pixel + kh - max_pad]);

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = kernel_size - pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = kernel[kv*kernel_size + kh - 1];
}

void SippBaseFilt::MiddleExecutionH(uint8_t **src, uint16_t *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = U8F_TO_U12F(src[kv][crt_pixel + kh - max_pad]);
}

void SippBaseFilt::buildKernel (uint16_t **src, uint16_t *kernel, int kernel_size, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(uint16_t **src, uint16_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][0];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::PadAtRight(uint16_t **src, uint16_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size - pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = kernel_size - pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = kernel[kv*kernel_size + kh - 1];
}

void SippBaseFilt::MiddleExecutionH(uint16_t **src, uint16_t *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::buildKernel (fp16 **src, fp16 *kernel, int kernel_size, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(fp16 **src, fp16 *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;
	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][0];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::PadAtRight(fp16 **src, fp16 *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size - pad_pixels; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = kernel_size - pad_pixels; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = kernel[kv*kernel_size + kh - 1];
}

void SippBaseFilt::MiddleExecutionH(fp16 **src, fp16 *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < kernel_size; kv++)
		for (kh = 0; kh < kernel_size; kh++)
			kernel[kv*kernel_size + kh] = src[kv][crt_pixel + kh - max_pad];
}

void SippBaseFilt::buildKernel (uint8_t *src, uint8_t *kernel, int kernel_size, int x) {
	// Determine the needed padding value
	int max_pad = (kernel_size >> 1);

	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(uint8_t *src, uint8_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < pad_pixels; kh++)
		kernel[kh] = src[0];

	for ( ; kh < kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];
}

void SippBaseFilt::PadAtRight(uint8_t *src, uint8_t *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < kernel_size - pad_pixels; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];

	for ( ; kh < kernel_size; kh++)
		kernel[kh] = kernel[kh - 1];
}

void SippBaseFilt::MiddleExecutionH(uint8_t *src, uint8_t *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];
}

void SippBaseFilt::buildKernel (fp16 *src, fp16 *kernel, int kernel_size, int x) {
	// Determine the needed padding value
	int max_pad = (kernel_size >> 1);

	// Horizontal padding
	int pad_pixels;

	if(x < max_pad)
		pad_pixels = max_pad - x;
	else if(x > width - max_pad - 1)
		pad_pixels = (width - 1) - (x + max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		PadAtLeft(src, kernel, kernel_size, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		PadAtRight(src, kernel, kernel_size, -pad_pixels, x);
	else
		MiddleExecutionH(src, kernel, kernel_size, x);
}

void SippBaseFilt::PadAtLeft(fp16 *src, fp16 *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < pad_pixels; kh++)
		kernel[kh] = src[0];

	for ( ; kh < kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];
}

void SippBaseFilt::PadAtRight(fp16 *src, fp16 *kernel, int kernel_size, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < kernel_size - pad_pixels; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];

	for ( ; kh < kernel_size; kh++)
		kernel[kh] = kernel[kh - 1];
}

void SippBaseFilt::MiddleExecutionH(fp16 *src, fp16 *kernel, int kernel_size, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int max_pad = kernel_size >> 1;
	int kh = 0;
	for ( ; kh < kernel_size; kh++)
		kernel[kh] = src[crt_pixel + kh - max_pad];
}

void SippBaseFilt::convertKernel(uint8_t *src, fp16 *dst, int size) {
	// Copy/convert uint8_t kernel with data in range [0, 255] to fp16 kernel with data in range [0, 1.0]
	for (int i = 0; i < size; i++)
		dst[i] = U8F_TO_FP16(src[i]);
}

void SippBaseFilt::convertKernel(fp16 *src, uint16_t *dst, int size) {
	// Copy/convert fp16 kernel with data in range [0, 1.0] to u12f kernel with data in range [0, 4095]
	for (int i = 0; i < size; i++)
		dst[i] = FP16_TO_U12F(src[i]);
}

void SippBaseFilt::convertKernel(uint8_t *src, uint16_t *dst, int size) {
	// Cast the u8 kernel to an u16 one
	for (int i = 0; i < size; i++)
		dst[i] = (uint16_t)(src[i]);
}

void SippBaseFilt::setOutputLine(uint8_t **out_ptr, int plane) {
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	// Determine in what slice does the current output plane start
	ComputeOutputStartSlice(plane);

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat()) * output_plane_start_slice;
	start_slice_address  =  (uint8_t *)out.buffer + sstride;

	*out_ptr = start_slice_address + out.GetOffset() 
		+ lstride * out.GetBufferIdx() 
		+ pstride * plane;
}

void SippBaseFilt::setOutputLine(uint16_t **out_ptr, int plane) {
	int lstride, pstride, sstride;
	uint16_t *start_slice_address;

	// Determine in what slice does the current output plane start
	ComputeOutputStartSlice(plane);

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat()) * output_plane_start_slice;
	start_slice_address  = (uint16_t *)out.buffer + sstride;

	*out_ptr = start_slice_address + out.GetOffset() 
		+ lstride * out.GetBufferIdx() 
		+ pstride * plane;
}

void SippBaseFilt::setOutputLine(uint32_t **out_ptr, int plane) {
	int lstride, pstride, sstride;
	uint32_t *start_slice_address;

	// Determine in what slice does the current output plane start
	ComputeOutputStartSlice(plane);

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat()) * output_plane_start_slice;
	start_slice_address  = (uint32_t *)out.buffer + sstride;

	*out_ptr = start_slice_address + out.GetOffset() 
		+ lstride * out.GetBufferIdx() 
		+ pstride * plane;
}

void SippBaseFilt::setOutputLine(fp16 **out_ptr, int plane) {
	int lstride, pstride, sstride;
	fp16 *start_slice_address;

	// Determine in what slice does the current output plane start
	ComputeOutputStartSlice(plane);

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat()) * output_plane_start_slice;
	start_slice_address    =   (fp16 *)out.buffer + sstride;

	*out_ptr = start_slice_address + out.GetOffset() 
		+ lstride * out.GetBufferIdx() 
		+ pstride * plane;
}

void SippBaseFilt::setOutputLine(float **out_ptr, int plane) {
	int lstride, pstride, sstride;
	float *start_slice_address;

	// Determine in what slice does the current output plane start
	ComputeOutputStartSlice(plane);

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat()) * output_plane_start_slice;
	start_slice_address    =  (float *)out.buffer + sstride;

	*out_ptr = start_slice_address + out.GetOffset() 
		+ lstride * out.GetBufferIdx() 
		+ pstride * plane;
}

void SippBaseFilt::setOutputLines(uint8_t **out_ptr, int number_of_planes) {
	int lstride, pstride, sstride;
	uint8_t *start_slice_address;

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  =  g_slice_size         / out.GetFormat();

	for(int pl = 0; pl <= number_of_planes; pl++) {
		ComputeOutputStartSlice(pl);
		start_slice_address = (uint8_t *)out.buffer + sstride * output_plane_start_slice;

		out_ptr[pl] = start_slice_address + out.GetOffset() 
			+ lstride * out.GetBufferIdx() 
			+ pstride * pl;
	}
}

void SippBaseFilt::setOutputLines(uint16_t **out_ptr, int number_of_planes) {
	int lstride, pstride, sstride;
	uint16_t *start_slice_address;

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  =  g_slice_size         / out.GetFormat();

	for(int pl = 0; pl <= number_of_planes; pl++) {
		ComputeOutputStartSlice(pl);
		start_slice_address = (uint16_t *)out.buffer + sstride * output_plane_start_slice;

		out_ptr[pl] = start_slice_address + out.GetOffset() 
			+ lstride * out.GetBufferIdx() 
			+ pstride * pl;
	}
}

void SippBaseFilt::setOutputLines(fp16 **out_ptr, int number_of_planes) {
	int lstride, pstride, sstride;
	fp16 *start_slice_address;

	lstride  =  out.GetLineStride () / out.GetFormat();
	pstride  = (out.GetPlaneStride() / out.GetFormat()) % g_slice_size;
	sstride  = (g_slice_size         / out.GetFormat());

	for(int pl = 0; pl <= number_of_planes; pl++) {
		ComputeOutputStartSlice(pl);
		start_slice_address = (fp16 *)out.buffer + sstride * output_plane_start_slice;

		out_ptr[pl] = start_slice_address + out.GetOffset() 
			+ lstride * out.GetBufferIdx() 
			+ pstride * pl;
	}
}

int SippBaseFilt::NextInputSlice(int offset) {
	return ( input_plane_start_slice + offset == g_last_slice + 1) ? g_first_slice - g_last_slice : 1;
}

int SippBaseFilt::NextOutputSlice(int offset) {
	return (output_plane_start_slice + offset == g_last_slice + 1) ? g_first_slice - g_last_slice : 1;
}

void SippBaseFilt::ComputeInputStartSlice(int plane) {
	if(plane == 0) {
		input_plane_start_slice = in.GetStartSlice();
	} else {
		int pstride, sstride, ss_in_ps, wrap;
		pstride  = in.GetPlaneStride() / in.GetFormat();
		sstride  = g_slice_size        / in.GetFormat();
		ss_in_ps = pstride / sstride;

		if((wrap = (input_plane_start_slice + ss_in_ps - g_last_slice)) > 0)
			input_plane_start_slice  = g_first_slice + wrap - 1;
		else
			input_plane_start_slice += ss_in_ps;
	}
}

void SippBaseFilt::ComputeOutputStartSlice(int plane) {
	if(plane == 0) {
		output_plane_start_slice = out.GetStartSlice();
	} else {
		int pstride, sstride, ss_in_ps, wrap;
		pstride  = out.GetPlaneStride() / out.GetFormat();
		sstride  = g_slice_size         / out.GetFormat();
		ss_in_ps = pstride / sstride;

		if((wrap = (output_plane_start_slice + ss_in_ps - g_last_slice)) > 0)
			output_plane_start_slice  = g_first_slice + wrap - 1;
		else
			output_plane_start_slice += ss_in_ps;
	}
}

void SippBaseFilt::PackInputLines(uint8_t **split_src, int kernel_size, uint8_t **packed_src) {
	int chunk_size   = in.GetChunkSize() / in.GetFormat();
	int slice_stride = g_slice_size      / in.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)width/chunk_size;

		for(int ln = 0; ln < kernel_size; ln++) {
			uint8_t *l_split_src  = (uint8_t *) split_src[ln];
			uint8_t *l_packed_src = (uint8_t *)packed_src[ln];

			for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
				for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
				l_split_src += slice_stride * NextInputSlice(crt_slice + 1);
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

void SippBaseFilt::PackInputLines(uint16_t **split_src, int kernel_size, uint16_t **packed_src) {
	int chunk_size   = in.GetChunkSize() / in.GetFormat();
	int slice_stride = g_slice_size      / in.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)width/chunk_size;

		for(int ln = 0; ln < kernel_size; ln++) {
			uint16_t *l_split_src  = (uint16_t *) split_src[ln];
			uint16_t *l_packed_src = (uint16_t *)packed_src[ln];

			for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
				for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
				l_split_src += slice_stride * NextInputSlice(crt_slice + 1);
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

void SippBaseFilt::PackInputLines(uint32_t **split_src, int kernel_size, uint32_t **packed_src) {
	int chunk_size   = in.GetChunkSize() / in.GetFormat();
	int slice_stride = g_slice_size      / in.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)width/chunk_size;

		for(int ln = 0; ln < kernel_size; ln++) {
			uint32_t *l_split_src  = (uint32_t *) split_src[ln];
			uint32_t *l_packed_src = (uint32_t *)packed_src[ln];

			for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
				for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
				l_split_src += slice_stride * NextInputSlice(crt_slice + 1);
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

void SippBaseFilt::PackInputLines(fp16 **split_src, int kernel_size, fp16 **packed_src) {
	int chunk_size   = in.GetChunkSize() / in.GetFormat();
	int slice_stride = g_slice_size      / in.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)width/chunk_size;

		for(int ln = 0; ln < kernel_size; ln++) {
			fp16 *l_split_src  = (fp16 *) split_src[ln];
			fp16 *l_packed_src = (fp16 *)packed_src[ln];

			for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
				for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
					*l_packed_src++ = l_split_src[crt_pixel];
				l_split_src += slice_stride* NextInputSlice(crt_slice + 1);
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

void SippBaseFilt::SplitOutputLine(uint8_t *packed_dst, int output_width, uint8_t *split_dst) {
	uint8_t *l_packed_dst = (uint8_t *)packed_dst;
	uint8_t *l_split_dst  = (uint8_t *) split_dst;
	int chunk_size   = out.GetChunkSize() / out.GetFormat();
	int slice_stride = g_slice_size       / out.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)output_width/chunk_size;

		for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
			for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
			l_split_dst += slice_stride * NextOutputSlice(crt_slice + 1);
		}
		if(int remainder = (output_width - full_slices_no * chunk_size))
			for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
	} else {
		for(int crt_pixel = 0; crt_pixel < output_width; crt_pixel++)
			*split_dst++ = *packed_dst++;
	}
}

void SippBaseFilt::SplitOutputLine(uint16_t *packed_dst, int output_width, uint16_t *split_dst) {
	uint16_t *l_packed_dst = (uint16_t *)packed_dst;
	uint16_t *l_split_dst  = (uint16_t *) split_dst;
	int chunk_size   = out.GetChunkSize() / out.GetFormat();
	int slice_stride = g_slice_size       / out.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)output_width/chunk_size;

		for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
			for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
			l_split_dst += slice_stride * NextOutputSlice(crt_slice + 1);
		}
		if(int remainder = (output_width - full_slices_no * chunk_size))
			for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
	} else {
		for(int crt_pixel = 0; crt_pixel < output_width; crt_pixel++)
			*split_dst++ = *packed_dst++;
	}
}

void SippBaseFilt::SplitOutputLine(uint32_t *packed_dst, int output_width, uint32_t *split_dst) {
	uint32_t *l_packed_dst = (uint32_t *)packed_dst;
	uint32_t *l_split_dst  = (uint32_t *) split_dst;
	int chunk_size   = out.GetChunkSize() / out.GetFormat();
	int slice_stride = g_slice_size       / out.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)output_width/chunk_size;

		for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
			for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
			l_split_dst += slice_stride * NextOutputSlice(crt_slice + 1);
		}
		if(int remainder = (output_width - full_slices_no * chunk_size))
			for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
	} else {
		for(int crt_pixel = 0; crt_pixel < output_width; crt_pixel++)
			*split_dst++ = *packed_dst++;
	}
}

void SippBaseFilt::SplitOutputLine(fp16 *packed_dst, int output_width, fp16 *split_dst) {
	fp16 *l_packed_dst = (fp16 *)packed_dst;
	fp16 *l_split_dst  = (fp16 *) split_dst;
	int chunk_size   = out.GetChunkSize() / out.GetFormat();
	int slice_stride = g_slice_size       / out.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)output_width/chunk_size;

		for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
			for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
			l_split_dst += slice_stride * NextOutputSlice(crt_slice + 1);
		}
		if(int remainder = (output_width - full_slices_no * chunk_size))
			for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
	} else {
		for(int crt_pixel = 0; crt_pixel < output_width; crt_pixel++)
			*split_dst++ = *packed_dst++;
	}
}

void SippBaseFilt::SplitOutputLine(float *packed_dst, int output_width, float *split_dst) {
	float *l_packed_dst = (float *)packed_dst;
	float *l_split_dst  = (float *) split_dst;
	int chunk_size   = out.GetChunkSize() / out.GetFormat();
	int slice_stride = g_slice_size       / out.GetFormat();

	if(chunk_size) {
		int full_slices_no = (int)output_width/chunk_size;

		for(int crt_slice = 0; crt_slice < full_slices_no; crt_slice++) {
			for(int crt_pixel = 0; crt_pixel < chunk_size; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
			l_split_dst += slice_stride * NextOutputSlice(crt_slice + 1);
		}
		if(int remainder = (output_width - full_slices_no * chunk_size))
			for(int crt_pixel = 0; crt_pixel < remainder; crt_pixel++)
				l_split_dst[crt_pixel] = *l_packed_dst++;
	} else {
		for(int crt_pixel = 0; crt_pixel < output_width; crt_pixel++)
			*split_dst++ = *packed_dst++;
	}
}

void SippBaseFilt::SetCmxBasePointer(void* cmxPtr) {
    in.cmxPtr = cmxPtr;
    out.cmxPtr = cmxPtr;
}

void SippBaseFilt::SetDdrBasePointer(void* ddrPtr) {
    in.ddrPtr = ddrPtr;
    out.ddrPtr = ddrPtr;
}

float SippBaseFilt::Interpolate(float val, float xa, float xb, float ya, float yb) {
	fp16 temp1 = val - xa;
	fp16 temp2 = xb  - xa;
	fp16 temp3 = yb  - ya;
	fp16 temp4 = ya;

	temp1 /= temp2;
	temp1 *= temp3;
	temp1 += temp4;

	return temp1;
}

float SippBaseFilt::ClampWR(float in, float min, float max) {
	if (in <= min) return min;
	if (in  > max) return max;
	else           return in;
}

int SippBaseFilt::ClampWR(int in, int min, int max) {
	if (in <= min) return min;
	if (in  > max) return max;
	else           return in;
}

int SippBaseFilt::GetMsb(int x) {
	if(!x)
		return x;
	for(int b = 0; x; x>>=1, b++)
		if(x == 0x1)
			return b;
	return -1;
}
