// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Debayer filter
//
//
// -----------------------------------------------------------------------------

#include "sippDebayer.h"

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


DebayerFilt::DebayerFilt(SippIrq *pObj, int id, int ahdk, int bilk, std::string name) :
	ahd_kernel_size (ahdk),
	bil_kernel_size (bilk) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);

#if GENERATE_MATRIX_COEFFICIENTS
		// Generate 16-bit matrix co-efficients
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++) {
				xyz_cam[i][j] = 0;
				for (int k = 0; k < 3; k++)
					xyz_cam[i][j] += (uint16_t)(xyz_rgb[i][k] * rgb_cam[k][j] * xyz_scale * 65536);
			}
#else
#if DONT_ROUND
		xyz_cam[0][0] = 0x60f9;
		xyz_cam[1][0] = 0x3200;
		xyz_cam[2][0] = 0x048b;
		xyz_cam[0][1] = 0x5413;
		xyz_cam[1][1] = 0xa826;
		xyz_cam[2][1] = 0x1c06;
		xyz_cam[0][2] = 0x2a6b;
		xyz_cam[1][2] = 0x10f7;
		xyz_cam[2][2] = 0xdf6b;
#else
		xyz_cam[0][0] = 0x6100;
		xyz_cam[1][0] = 0x3200;
		xyz_cam[2][0] = 0x0500;
		xyz_cam[0][1] = 0x5400;
		xyz_cam[1][1] = 0xa800;
		xyz_cam[2][1] = 0x1c00;
		xyz_cam[0][2] = 0x2a00;
		xyz_cam[1][2] = 0x1100;
		xyz_cam[2][2] = 0xe000;
#endif
#endif
		crt_output_sp = 0;
}

void DebayerFilt::SetBayerPattern(int val, int reg) {
	if(!strchr(possible_bayer_patterns, (char)(val + '0'))) {
		std::cerr << "Invalid bayer pattern: possible pattern are 0(GRBG), 1(RGGB), 2(GBRG), 3(BGGR)." << std::endl;
		abort();
	}
	dbyr_params[reg].bayer_pattern = val;
}

void DebayerFilt::SetLumaOnly(int val, int reg) {
	dbyr_params[reg].luma_only = val;
}

void DebayerFilt::SetForceRbToZero(int val, int reg) {
	dbyr_params[reg].force_rb_to_zero = val;
}

void DebayerFilt::SetInputDataWidth(int val, int reg) {
	dbyr_params[reg].input_data_width = val;
}

void DebayerFilt::SetOutputDataWidth(int val, int reg) {
	dbyr_params[reg].output_data_width = val;
	output_shift = 16 - (dbyr_params[reg].output_data_width + 1);
}

void DebayerFilt::SetImageOrderOut(int val, int reg) {
	dbyr_params[reg].image_order_out = val;
	DeterminePlanes();
}

void DebayerFilt::SetPlaneMultiple(int val, int reg) {
	dbyr_params[reg].plane_multiple = val;

	int actual_np_o = out.GetNumPlanes() + 1;
	int actual_pm   = dbyr_params[reg].plane_multiple     + 1;
	full_set  = actual_np_o / actual_pm;
	remainder = actual_np_o - actual_pm * full_set;
}

void DebayerFilt::SetGradientMul(int val, int reg) {
	dbyr_params[reg].gradient_mul.full = val;
}

void DebayerFilt::SetThreshold1(int val, int reg) {
	dbyr_params[reg].th1 = val;
}

void DebayerFilt::SetThreshold2(int val, int reg) {
	dbyr_params[reg].th2 = val;
}

void DebayerFilt::SetSlope(int val, int reg) {
	dbyr_params[reg].slope.full = val;
}

void DebayerFilt::SetOffset(int val, int reg) {
	dbyr_params[reg].offset = val;
}

void DebayerFilt::DeterminePlanes() {
	switch(dbyr_params[reg].image_order_out) {
	case P_RGB :
		planes[0] = 0; planes[1] = 1; planes[2] = 2;
		break;
	case P_BGR :
		planes[0] = 2; planes[1] = 1; planes[2] = 0;
		break;
	case P_RBG :
		planes[0] = 0; planes[1] = 2; planes[2] = 1;
		break;
	case P_BRG :
		planes[0] = 2; planes[1] = 0; planes[2] = 1;
		break;
	case P_GRB :
		planes[0] = 1; planes[1] = 0; planes[2] = 2;
		break;
	case P_GBR :
		planes[0] = 1; planes[1] = 2; planes[2] = 0;
		break;
	}
}

void DebayerFilt::PromoteTo16Bits(uint8_t  *src, uint16_t *src_16, int size) {
	int actual_width = input_data_width + 1;

	switch(actual_width) {
	case 6 :
	case 7 :
		for (int i = 0; i < size; i++)
			src_16[i] = (uint16_t)(src[i] << ( 16 - actual_width * 1) | 
			                       src[i] << ( 16 - actual_width * 2) |
			                       src[i] >> (-16 + actual_width * 3)
			                       );
		break;
	default :
		for (int i = 0; i < size; i++)
			src_16[i] = (uint16_t)(src[i] << (16 - actual_width) | src[i] >> ((actual_width << 1) - 16));
		break;
	}
}

void DebayerFilt::PromoteTo16Bits(uint16_t *src, uint16_t *src_16, int size) {
	for (int i = 0; i < size; i++)
		src_16[i] = (uint16_t)(src[i] << (16 - (input_data_width + 1)) | src[i] >> (((input_data_width + 1) << 1) - 16));
}

void DebayerFilt::GetMaximumGradient(uint16_t **src, uint32_t *local_grad, int ln, int pix) {
	uint16_t grads[6];

	grads[0] = abs(src[ln - 1][pix - 1] - src[ln - 1][pix + 1]);
	grads[1] = abs(src[  ln  ][pix - 1] - src[  ln  ][pix + 1]);
	grads[2] = abs(src[ln + 1][pix - 1] - src[ln + 1][pix + 1]);
	grads[3] = abs(src[ln - 1][pix - 1] - src[ln + 1][pix - 1]);
	grads[4] = abs(src[ln - 1][  pix  ] - src[ln + 1][  pix  ]);
	grads[5] = abs(src[ln - 1][pix + 1] - src[ln + 1][pix + 1]);

	uint16_t max = MAX(MAX(grads[0], grads[1]), MAX(MAX(grads[2], grads[3]), MAX(grads[4], grads[5])));
	local_grad[pix] = DOWNCAR((max * gradient_mul.full), 7);
}

void DebayerFilt::BilinearDebayer(uint16_t **src, uint16_t *local_luma, uint32_t *local_grad, uint16_t *rgb_bil[3]) {
	uint16_t val;

	// Determine current line index and type
	int    ln = BIL_SP;
	int color = RL(bayer_pattern, ln) ? 0 : 2;

	for(int pix = BIL_SP; pix < width - BIL_SP; pix++) {
		uint32_t converted_y = 0;
		for(int i = ln - 1, lb_i = 0; i < ln + bil_kernel_size - 1; i++)
			for(int j = pix - 1; j < pix + bil_kernel_size - 1; j++)
				// Get local brightness directly from bayer
				converted_y += src[i][j] * luma_bayer[lb_i++];
		
		// Scale the local luma estimation
		local_luma[pix] = converted_y >> 4;

		// Get maximum local gradient
		GetMaximumGradient(src, local_grad, ln, pix);

		// Neither blue nor red pixel
		if(!RP(bayer_pattern, ln, pix) && !BP(bayer_pattern, ln, pix)) {
			// Red/blue channel interpolation at original green location
			val = (src[ln][pix - 1] + src[ln][pix + 1]) >> 1;
			rgb_bil[color ^ 0][pix] = val;
			val = (src[ln - 1][pix] + src[ln + 1][pix]) >> 1;
			rgb_bil[color ^ 2][pix] = val;

			// Preserve the current green value
			rgb_bil[1][pix] = src[ln][pix];
		} else {
			// Red/blue and green channel interpolation at original blue/red location
			val = (src[ln - 1][pix - 1]           +          src[ln - 1][pix + 1]
			                                      +  
			       src[ln + 1][pix - 1]           +          src[ln + 1][pix + 1]) >> 2;
			rgb_bil[color ^ 2][pix] = val;
			val = (                     src[ln - 1][pix + 0]
			    +  src[ln + 0][pix - 1]           +          src[ln + 0][pix + 1] +
			                            src[ln + 1][pix + 0]                     ) >> 2;
			rgb_bil[1][pix] = val;

			// Preserve the current red/blue value
			rgb_bil[color ^ 0][pix] = src[ln][pix];
		}
	}
}

void DebayerFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	bayer_pattern     = dbyr_params[reg].bayer_pattern;
	luma_only         = dbyr_params[reg].luma_only;
	force_rb_to_zero  = dbyr_params[reg].force_rb_to_zero;
	input_data_width  = dbyr_params[reg].input_data_width;
	output_data_width = dbyr_params[reg].output_data_width;
	image_order_out   = dbyr_params[reg].image_order_out;
	plane_multiple    = dbyr_params[reg].plane_multiple;
	gradient_mul      = dbyr_params[reg].gradient_mul;
	th1               = dbyr_params[reg].th1;
	th2               = dbyr_params[reg].th2;
	slope             = dbyr_params[reg].slope;
	offset            = dbyr_params[reg].offset;
}

void DebayerFilt::SetUpAndRun(void) {
	static int count = 0;
	int dec = full_set;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint8_t  **     out_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint16_t **     out_ptr_16 = 0;

	void **in_ptr;
	void **out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  *[ahd_kernel_size];
		      in_ptr_8  = new uint8_t  *[ahd_kernel_size];
		for(int ln = 0; ln < ahd_kernel_size; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[ahd_kernel_size];
		      in_ptr_16 = new uint16_t *[ahd_kernel_size];
		for(int ln = 0; ln < ahd_kernel_size; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
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
		// Determine the number of output planes
		// corresponding to the current input Bayer plane
		if(dec)
			crt_output_planes = plane_multiple;
		else
			crt_output_planes = remainder - 1;

		// Prepare filter buffer line pointers then run
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_8 , ahd_kernel_size, pl);
			PackInputLines(split_in_ptr_8 , ahd_kernel_size, in_ptr_8 );
		} else {
			getKernelLines(split_in_ptr_16, ahd_kernel_size, pl);
			PackInputLines(split_in_ptr_16, ahd_kernel_size, in_ptr_16);
		}

		Run(in_ptr, out_ptr);
		dec--;
		crt_output_sp += plane_multiple + 1;
	}
	// Clear start bit, update buffer fill levels, input frame 
	// line index and reset the plane starting point
	in.ClrStartBit();
	UpdateBuffersSyncNoPad(ahd_kernel_size);
	IncLineIdxNoPad(ahd_kernel_size);
	crt_output_sp = 0;

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;

	delete[]  in_ptr;
	delete[] out_ptr;
}

void DebayerFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint8_t  **     out_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint16_t **     out_ptr_16 = 0;

	void **in_ptr;
	void **out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  *[ahd_kernel_size];
		      in_ptr_8  = new uint8_t  *[ahd_kernel_size];
		for(int ln = 0; ln < ahd_kernel_size; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[ahd_kernel_size];
		      in_ptr_16 = new uint16_t *[ahd_kernel_size];
		for(int ln = 0; ln < ahd_kernel_size; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
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
	while (CanRunNoPad(ahd_kernel_size)) {
		int dec = full_set;

		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Prepare filter output buffer line pointers
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			setOutputLines(out_ptr_8 , out.GetNumPlanes());
		else
			setOutputLines(out_ptr_16, out.GetNumPlanes());
		
		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Determine the number of output planes
			// corresponding to the current input Bayer plane
			if(dec)
				crt_output_planes = plane_multiple;
			else
				crt_output_planes = remainder - 1;

			// Prepare filter input buffer line pointers then run
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_in_ptr_8 , ahd_kernel_size, pl);
				PackInputLines(split_in_ptr_8 , ahd_kernel_size, in_ptr_8 );
			} else {
				getKernelLines(split_in_ptr_16, ahd_kernel_size, pl);
				PackInputLines(split_in_ptr_16, ahd_kernel_size, in_ptr_16);
			}

			Run(in_ptr, out_ptr);
			dec--;
			crt_output_sp += plane_multiple + 1;
		}
		// Update buffer fill levels, input frame line 
		// index and reset the plane starting point
		UpdateBuffersNoPad(ahd_kernel_size);
		IncLineIdxNoPad(ahd_kernel_size);
		crt_output_sp = 0;

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

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;

	delete[]  in_ptr;
	delete[] out_ptr;
}

void DebayerFilt::Run(void **in_ptr, void **out_ptr) {
	uint8_t  **       src_8  = 0;
	uint8_t  **       dst_8  = 0;
	uint8_t  **packed_dst_8  = 0;
	uint16_t **       src    = 0;
	uint16_t **       src_16 = 0;
	uint16_t **       dst_16 = 0;
	uint16_t **packed_dst_16 = 0;

	uint16_t **rgb[2][3];
	uint16_t  *rgb_bil[3];
	uint16_t *local_luma;
	uint32_t *local_grad;
	short **lab[2][3];
	char **homo[2];
	int hm[2];
	unsigned int ldiff[2][4], abdiff[2][4], leps, abeps;
	static const int dir[2] = {-1, 1};

	// Allocating array of pointers to to input lines
	// and setting input pointers
	src = new uint16_t *[ahd_kernel_size];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_8 = new uint8_t *[ahd_kernel_size];
		for (int i = 0; i < ahd_kernel_size; i++)
			src_8[i] = (uint8_t *)in_ptr[i];
		// Promote 8bit data to 16 bits
		for (int i = 0; i < ahd_kernel_size; i++) {
			src[i] = new uint16_t[width];
			PromoteTo16Bits(src_8[i], src[i], width);
		}
	} else {
		src_16 = new uint16_t *[ahd_kernel_size];
		for (int i = 0; i < ahd_kernel_size; i++)
			src_16[i] = (uint16_t *)in_ptr[i];
		// Promote 9 or higher bit data to 16 bits
		for (int i = 0; i < ahd_kernel_size; i++) {
			src[i] = new uint16_t[width];
			PromoteTo16Bits(src_16[i], src[i], width);
		}
	}

	// Allocating array of pointers to to output lines, setting output
	// pointers and allocating temporary packed planar output buffers
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_8  = new uint8_t  *[crt_output_planes + 1];
		packed_dst_8  = new uint8_t  *[crt_output_planes + 1];
		for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + crt_output_planes; pl++) {
			       dst_8 [lpl  ]  = (uint8_t  *)out_ptr[pl];
			packed_dst_8 [lpl++]  = new uint8_t [width - ahd_kernel_size + 1];
		}
	} else {
		       dst_16 = new uint16_t *[crt_output_planes + 1];
		packed_dst_16 = new uint16_t *[crt_output_planes + 1];
		for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + crt_output_planes; pl++) {
			       dst_16[lpl  ]  = (uint16_t *)out_ptr[pl];
			packed_dst_16[lpl++]  = new uint16_t[width - ahd_kernel_size + 1];
		}
	}

	// Allocate temporary images used for vertical/horizontal interpolation
	for(int d = 0; d < 2; d++)
		for(int color = 0; color < 3; color++) {
			rgb[d][color] = new uint16_t *[ahd_kernel_size];
			for(int h = 0; h < ahd_kernel_size; h++)
				rgb[d][color][h] = new uint16_t[width];
		}

	// Allocate buffer for conversion to CIELAB
	for(int d = 0; d < 2; d++)
		for(int color = 0; color < 3; color++) {
			lab[d][color] = new short *[ahd_kernel_size - 6];
			for(int h = 0; h < ahd_kernel_size - 6; h++)
				lab[d][color][h] = new short[width];
		}

	// Allocate and initialize homogeneity map
	for(int d = 0; d < 2; d++) {
		homo[d] = new char *[ahd_kernel_size - 8];
		for(int h = 0; h < ahd_kernel_size - 8; h++) {
			homo[d][h] = new char[width];
			memset(homo[d][h], 0, width*sizeof(***homo));
		}
	}

	// Allocate temporary buffers for bilinear demosaicing,
	// local luma estimation and maximum local gradient
	for(int color = 0; color < 3; color++)
		rgb_bil[color] = new uint16_t[width];
	local_luma = new uint16_t[width];
	local_grad = new uint32_t[width];

	int start_pixel;
	int val;
	uint8_t  alpha;
	uint32_t xyz[3];

	// Force RB to zero
	if(force_rb_to_zero) {
		for(int i = 0; i < ahd_kernel_size; i++)
			for(int j = 0; j < width; j++)
				if(RP(bayer_pattern, i, j) || BP(bayer_pattern, i, j))
					src[i][j] = 0;
	}

	// Bilinear demosaicing
	BilinearDebayer(src, local_luma, local_grad, rgb_bil);

	// Green channel interpolation
	for(int i = GI_SP; i < ahd_kernel_size - GI_SP; i++) {
		// Determine starting position
		if(bayer_pattern == GRBG || bayer_pattern == BGGR)
			start_pixel = (RL(bayer_pattern, i)) ? 3 : 2;
		else
			start_pixel = (RL(bayer_pattern, i)) ? 2 : 3;

		for(int j = start_pixel; j < width - GI_SP; j += 2) {
			// Horizontal interpolation
			val = ((src[i][j - 1] + src[i][j] + src[i][j + 1]) *  2
			      - src[i][j - 2]             - src[i][j + 2]) >> 2;
			rgb[0][1][i][j] = (uint16_t)ULIM(val, src[i][j - 1], src[i][j + 1]);

			// Vertical interpolation
			val = ((src[i - 1][j] + src[i][j]  + src[i + 1][j]) *  2
			      - src[i - 2][j]              - src[i + 2][j]) >> 2;
			rgb[1][1][i][j] = (uint16_t)ULIM(val, src[i - 1][j], src[i + 1][j]);
		}
	}
	// Red/blue channel interpolation
	for(int d = 0; d < 2; d++) {
		for(int i = RBI_SP; i < ahd_kernel_size - RBI_SP; i++) {
			for(int j = RBI_SP; j < width - RBI_SP; j++) {
				// Either on blue or red pixel
				if(BP(bayer_pattern, i, j) || RP(bayer_pattern, i, j)) {
					// Red/blue channel interpolation at original blue/RED location (and interpolated green)
					val = rgb[d][1][i][j] + ((       src[i - 1][j - 1] +       src[i - 1][j + 1]
					                         +       src[i + 1][j - 1] +       src[i + 1][j + 1]
					                         - rgb[d][1][i - 1][j - 1] - rgb[d][1][i - 1][j + 1]
					                         - rgb[d][1][i + 1][j - 1] - rgb[d][1][i + 1][j + 1]) >> 2);
				} else {
					// Red/blue channel interpolation at original green location
					val = src[i][j] + ((       src[i][j - 1] +       src[i][j + 1]
						               - rgb[d][1][i][j - 1] - rgb[d][1][i][j + 1]) >> 1);
					rgb[d][RL(bayer_pattern, i) ? 0 : 2][i][j] = (uint16_t)CLIP(val, 0xffff);
					val = src[i][j] + ((       src[i - 1][j] +       src[i + 1][j]
						               - rgb[d][1][i - 1][j] - rgb[d][1][i + 1][j]) >> 1);
				}

				// Update missing color from the line
				rgb[d][BL(bayer_pattern, i) ? 0 : 2][i][j] = (uint16_t)CLIP(val, 0xffff);

				// Preserve current pixel in the respective image plane
				rgb[d][RP(bayer_pattern, i, j) ? 0 : BP(bayer_pattern, i, j) ? 2 : 1][i][j] = src[i][j];

				// Compute xyz values
				xyz[0] = xyz[1] = xyz[2] = 0;
				for(int c = 0; c < 3; c++) {
					xyz[0] += ((uint32_t)xyz_cam[0][c] * rgb[d][c][i][j]) >> 16;
					xyz[1] += ((uint32_t)xyz_cam[1][c] * rgb[d][c][i][j]) >> 16;
					xyz[2] += ((uint32_t)xyz_cam[2][c] * rgb[d][c][i][j]) >> 16;
				}

				// Convert to CIELAB space
				lab[d][0][i - RBI_SP][j - RBI_SP] = (rgb[d][0][i][j] + rgb[d][1][i][j] + rgb[d][2][i][j]) >> 6;
				lab[d][1][i - RBI_SP][j - RBI_SP] = (short)((xyz[0] - xyz[1]) >> 6);
				lab[d][2][i - RBI_SP][j - RBI_SP] = (short)((xyz[1] - xyz[2]) >> 7);
			}
		}
	}
	for(int i = HM_SP; i < HM_LINES + HM_SP; i++) {
		for(int j = HM_SP; j < width - 2*RBI_SP - HM_SP; j++) {
			for(int d = 0; d < 2; d++)
				for(int k = 0; k < 4; k++)
					// Sum the square of the difference with the neighbour LAB values in each cardinal direction
					if(k < 2) {
						ldiff [d][k] = ABS(lab[d][0][i][j] - lab[d][0][i][j + dir[k]]);
						abdiff[d][k] = SQR(lab[d][1][i][j] - lab[d][1][i][j + dir[k]])
						             + SQR(lab[d][2][i][j] - lab[d][2][i][j + dir[k]]);
					} else {
						ldiff [d][k] = ABS(lab[d][0][i][j] - lab[d][0][i + dir[k%2]][j]);
						abdiff[d][k] = SQR(lab[d][1][i][j] - lab[d][1][i + dir[k%2]][j])
						             + SQR(lab[d][2][i][j] - lab[d][2][i + dir[k%2]][j]);
					}

			// Determine the minimum of the maximum values in each direction (horizontal/vertical)
			leps  = MIN(MAX( ldiff[0][0], ldiff[0][1]),
			            MAX( ldiff[1][2], ldiff[1][3]));
			abeps = MIN(MAX(abdiff[0][0],abdiff[0][1]),
			            MAX(abdiff[1][2],abdiff[1][3]));

			// Tally points
			for (int d = 0; d < 2; d++)
				for(int k = 0; k < 4; k++) {
					if(ldiff[d][k] <= leps && (luma_only || abdiff[d][k] <= abeps))
						homo[d][i - HM_SP][j]++;
					if(ABS(lab[d][1][i][j]) < th1 && ABS(lab[d][2][i][j]) < th2)
						homo[d][i - HM_SP][j]++;
				}
		}
	}

	int ii, jj;
	// Combine the most homogenous pixels for the final result
	for(int j = CHC_SP; j < width - 2*RBI_SP - CHC_SP; j++) {
		for(int d = 0; d < 2; d++) 
			for(hm[d] = 0, ii = 0; ii < HM_LINES; ii++) 
				for(jj = j - 1; jj <= j + 1; jj++)
					hm[d] += homo[d][ii][jj];
		for(int pl = 0; pl <= crt_output_planes; pl++) {
			// Compute and clamp the blending factor
			int64_t tmp1 = ((int64_t)local_luma[j + RBI_SP] +(int64_t)local_grad[j + RBI_SP] + (int64_t)offset) * (int64_t)slope.full;
			int32_t tmp = (int32_t)(tmp1 / 256);
			alpha = (uint8_t)((uint16_t)ClampWR((float)tmp, 0.f, (float)((1 << 16) - 1)) >> 8);

			// Fade towards bilinear interpolation in dark areas
			uint16_t ahd_out = rgb[hm[1] > hm[0]][planes[pl]][ahd_kernel_size >> 1][j + RBI_SP];
			uint16_t bil_out = rgb_bil[planes[pl]][j + RBI_SP];
			uint32_t out_val = DOWNCAR((alpha * (ahd_out - bil_out)), 8) + bil_out;

			// Scale and output the blended value
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_8 [pl][j - CHC_SP] = (uint8_t )(out_val >> output_shift);
			else
				packed_dst_16[pl][j - CHC_SP] = (uint16_t)(out_val >> output_shift);
		}
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		for(int pl = 0; pl <= crt_output_planes; pl++)
			SplitOutputLine(packed_dst_8 [pl], width - ahd_kernel_size + 1, dst_8 [pl]);
		delete[]        dst_8 ;
		delete[] packed_dst_8 ;
	} else {
		for(int pl = 0; pl <= crt_output_planes; pl++)
			SplitOutputLine(packed_dst_16[pl], width - ahd_kernel_size + 1, dst_16[pl]);
		delete[]        dst_16;
		delete[] packed_dst_16;
	}

	for(int d = 0; d < 2; d++) {
		for(int h = 0; h < ahd_kernel_size - 8; h++)
			delete[] homo[d][h];
		delete[] homo[d];
	}
	for(int d = 0; d < 2; d++) 
		for(int color = 0; color < 3; color++) {
			for(int h = 0; h < ahd_kernel_size - 6; h++)
				delete[] lab[d][color][h];
			delete[] lab[d][color];
			for(int h = 0; h < ahd_kernel_size    ; h++)
				delete[] rgb[d][color][h];
			delete[] rgb[d][color];
		}
	for(int color = 0; color < 3; color++)
		delete[] rgb_bil[color];
	delete[] local_luma;
	delete[] local_grad;

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] src_8;
	delete[] src;
}

// Not used in this particular filter as it has his own version of Run
void *DebayerFilt::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}
