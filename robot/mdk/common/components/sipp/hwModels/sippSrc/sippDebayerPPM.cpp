// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Debayer post-processing median filter
//
//
// -----------------------------------------------------------------------------

#include "sippDebayerPPM.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <climits>
#include <math.h>


DebayerPPMFilt::DebayerPPMFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);
}

void DebayerPPMFilt::SetDataWidth(int val, int reg) {
	dbyrppm_params[reg].data_width = val;
}

void DebayerPPMFilt::ComputeDifferences(uint16_t *minuend, uint16_t *subtrahend, int32_t *diff) {
	for(int pix = 0; pix < kernel_lines * kernel_lines; pix++)
		diff[pix] = minuend[pix] - subtrahend[pix];
}

void DebayerPPMFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	data_width = dbyrppm_params[reg].data_width;
}

void DebayerPPMFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  ***split_in_ptr_8  = 0;
	uint8_t  ***      in_ptr_8  = 0;
	uint8_t  **      out_ptr_8  = 0;
	uint16_t ***split_in_ptr_16 = 0;
	uint16_t ***      in_ptr_16 = 0;
	uint16_t **      out_ptr_16 = 0;

	void ***in_ptr;
	void **out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  **[in.GetNumPlanes() + 1];
		      in_ptr_8  = new uint8_t  **[in.GetNumPlanes() + 1];
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			split_in_ptr_8 [pl] = new uint8_t  *[kernel_lines];
			      in_ptr_8 [pl] = new uint8_t  *[kernel_lines];
			for(int ln = 0; ln < kernel_lines; ln++)
				in_ptr_8 [pl][ln] = new uint8_t [width];
		}
		in_ptr = (void ***)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t **[in.GetNumPlanes() + 1];
		      in_ptr_16 = new uint16_t **[in.GetNumPlanes() + 1];
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			split_in_ptr_16[pl] = new uint16_t *[kernel_lines];
			      in_ptr_16[pl] = new uint16_t *[kernel_lines];
			for(int ln = 0; ln < kernel_lines; ln++)
				in_ptr_16[pl][ln] = new uint16_t[width];
		}
		in_ptr = (void ***)in_ptr_16;
	}

	// Assign first dimension of pointers to output lines
	// and prepare filter output buffer line pointers
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		out_ptr_8  = new uint8_t  *[out.GetNumPlanes() + 1];
		setOutputLines(out_ptr_8 , out.GetNumPlanes());
		out_ptr = (void **)out_ptr_8;
	} else {
		out_ptr_16 = new uint16_t *[out.GetNumPlanes() + 1];
		setOutputLines(out_ptr_16, out.GetNumPlanes());
		out_ptr = (void **)out_ptr_16;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Filter a line from each plane
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Prepare filter buffer line pointers then run
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_8 [pl], kernel_lines, pl);
			PackInputLines(split_in_ptr_8 [pl], kernel_lines, in_ptr_8 [pl]);
		} else {
			getKernelLines(split_in_ptr_16[pl], kernel_lines, pl);
			PackInputLines(split_in_ptr_16[pl], kernel_lines, in_ptr_16[pl]);
		}
	}
	Run(in_ptr, out_ptr);

	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			delete[] split_in_ptr_8 [pl];
		} else {				   
			delete[] split_in_ptr_16[pl];
		}	
		delete[] in_ptr[pl];
	}
	delete[] out_ptr;
}

void DebayerPPMFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  ***split_in_ptr_8  = 0;
	uint8_t  ***      in_ptr_8  = 0;
	uint8_t  **      out_ptr_8  = 0;
	uint16_t ***split_in_ptr_16 = 0;
	uint16_t ***      in_ptr_16 = 0;
	uint16_t **      out_ptr_16 = 0;

	void ***in_ptr;
	void **out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  **[in.GetNumPlanes() + 1];
		      in_ptr_8  = new uint8_t  **[in.GetNumPlanes() + 1];
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			split_in_ptr_8 [pl] = new uint8_t  *[kernel_lines];
			      in_ptr_8 [pl] = new uint8_t  *[kernel_lines];
			for(int ln = 0; ln < kernel_lines; ln++)
				in_ptr_8 [pl][ln] = new uint8_t [width];
		}
		in_ptr = (void ***)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t **[in.GetNumPlanes() + 1];
		      in_ptr_16 = new uint16_t **[in.GetNumPlanes() + 1];
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			split_in_ptr_16[pl] = new uint16_t *[kernel_lines];
			      in_ptr_16[pl] = new uint16_t *[kernel_lines];
			for(int ln = 0; ln < kernel_lines; ln++)
				in_ptr_16[pl][ln] = new uint16_t[width];
		}
		in_ptr = (void ***)in_ptr_16;
	}

	// Assign first dimension of pointers to output lines
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		out_ptr_8  = new uint8_t  *[out.GetNumPlanes() + 1];
		out_ptr = (void **)out_ptr_8;
	} else {
		out_ptr_16 = new uint16_t *[out.GetNumPlanes() + 1];
		out_ptr = (void **)out_ptr_16;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_lines)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Prepare filter output buffer line pointers
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			setOutputLines(out_ptr_8 , out.GetNumPlanes());
		else
			setOutputLines(out_ptr_16, out.GetNumPlanes());
		
		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_in_ptr_8 [pl], kernel_lines, pl);
				PackInputLines(split_in_ptr_8 [pl], kernel_lines, in_ptr_8 [pl]);
			} else {
				getKernelLines(split_in_ptr_16[pl], kernel_lines, pl);
				PackInputLines(split_in_ptr_16[pl], kernel_lines, in_ptr_16[pl]);
			}
		}
		Run(in_ptr, out_ptr);

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
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			delete[] split_in_ptr_8 [pl];
		} else {				   
			delete[] split_in_ptr_16[pl];
		}	
		delete[] in_ptr[pl];
	}
	delete[] out_ptr;
}

void DebayerPPMFilt::Run(void ***in_ptr, void **out_ptr) {
	uint8_t  ***src_8        = 0;
	uint16_t ***src_16       = 0;
	uint16_t *kernels[3];
	uint8_t  **dst_8         = 0;
	uint8_t  **packed_dst_8  = 0;
	uint16_t **dst_16        = 0;
	uint16_t **packed_dst_16 = 0;

	// Allocating and setting input pointers
	src_16 = new uint16_t **[in.GetNumPlanes() + 1];
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
		src_16[pl] = new uint16_t *[kernel_lines];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_8 = new uint8_t **[in.GetNumPlanes() + 1];
		for(int pl = 0; pl <=  in.GetNumPlanes(); pl++) {
			src_8[pl] = new uint8_t *[kernel_lines];
			for(int l = 0; l < kernel_lines; l++) {
				src_8 [pl][l] = (uint8_t  *)in_ptr[pl][l];
				src_16[pl][l] = new uint16_t[width];
				convertKernel(src_8[pl][l], src_16[pl][l], width);
			}
			delete[] src_8[pl];
		}
		delete[] src_8;
	} else {
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
			for(int l = 0; l < kernel_lines; l++)
				src_16[pl][l] = (uint16_t *)in_ptr[pl][l];
	}

	// 2D kernels
	for(int pl = 0; pl < 3; pl++)
		kernels[pl] = new uint16_t[kernel_lines*kernel_lines];

	// Allocating array of pointers to to output lines, setting output
	// pointers and allocating temporary packed planar output buffers
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_8  = new uint8_t  *[out.GetNumPlanes() + 1];
		packed_dst_8  = new uint8_t  *[out.GetNumPlanes() + 1];
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++) {
			       dst_8 [pl] = (uint8_t  *)out_ptr[pl];
			packed_dst_8 [pl] = new uint8_t [width];
		}
	} else {
		       dst_16 = new uint16_t *[out.GetNumPlanes() + 1];
		packed_dst_16 = new uint16_t *[out.GetNumPlanes() + 1];
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++) {
			       dst_16[pl] = (uint16_t *)out_ptr[pl];
			packed_dst_16[pl] = new uint16_t[width];
		}
	}

	int32_t *rg = new int32_t[kernel_lines * kernel_lines];
	int32_t *bg = new int32_t[kernel_lines * kernel_lines];
	int32_t *gr = new int32_t[kernel_lines * kernel_lines];
	int32_t *gb = new int32_t[kernel_lines * kernel_lines];
	int32_t out_val[3];

	int x = 0;

	while (x < width) {
		// Build the 2D kernels
		for(int pl = 0; pl <= in.GetNumPlanes(); pl++)
			buildKernel(src_16[pl], kernels[pl], kernel_lines, x);

		// Compute planar differences
		ComputeDifferences(kernels[0], kernels[1], rg);
		ComputeDifferences(kernels[2], kernels[1], bg);
		ComputeDifferences(kernels[1], kernels[0], gr);
		ComputeDifferences(kernels[1], kernels[2], gb);

		// Sort the differences
		qsort((void *)rg, (size_t)(kernel_lines*kernel_lines), sizeof(int32_t), compare);
		qsort((void *)bg, (size_t)(kernel_lines*kernel_lines), sizeof(int32_t), compare);
		qsort((void *)gr, (size_t)(kernel_lines*kernel_lines), sizeof(int32_t), compare);
		qsort((void *)gb, (size_t)(kernel_lines*kernel_lines), sizeof(int32_t), compare);

		// Add back the median value
		out_val[0] =          (rg[KERNEL_CENTRE] + kernels[1][KERNEL_CENTRE]);
		out_val[2] =          (bg[KERNEL_CENTRE] + kernels[1][KERNEL_CENTRE]);
		out_val[1] = (int32_t)(gr[KERNEL_CENTRE] + kernels[0][KERNEL_CENTRE] + gb[KERNEL_CENTRE] + kernels[2][KERNEL_CENTRE]) / 2;

		// Clamp and output
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_8 [pl][x] = (uint8_t )ClampWR((float)out_val[pl], 0.f, (float)((1 << (data_width + 1)) - 1));
			else
				packed_dst_16[pl][x] = (uint16_t)ClampWR((float)out_val[pl], 0.f, (float)((1 << (data_width + 1)) - 1));
		x++;
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
			SplitOutputLine(packed_dst_8 [pl], width, dst_8 [pl]);
		delete[]        dst_8;
		delete[] packed_dst_8;
	} else {
		for(int pl = 0; pl <= out.GetNumPlanes(); pl++)
			SplitOutputLine(packed_dst_16[pl], width, dst_16[pl]);
		delete[]        dst_16;
		delete[] packed_dst_16;
	}

	delete[] rg; delete[] bg; 
	delete[] gr; delete[] gb;
	for(int pl = 0; pl < 3; pl++)
		delete[] kernels[pl];
	for(int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT)
			for(int l = 0; l < kernel_lines; l++)
				delete[] src_16[pl][l];
		delete[] src_16[pl];
	}
	delete[] src_16;
}

// Not used in this particular filter as it has his own version of Run
void *DebayerPPMFilt::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}

// Comparison function for qsort()
int compare(const void *a, const void *b) {
	int32_t *pa = (int32_t *)a;
	int32_t *pb = (int32_t *)b;
	return *pa == *pb ? 0 : *pa < *pb ? -1 : 1;
}
