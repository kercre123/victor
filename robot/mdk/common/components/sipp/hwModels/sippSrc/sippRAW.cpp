// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - RAW filter
//
//
// -----------------------------------------------------------------------------

#include "sippRAW.h"

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


RawFilt::RawFilt(SippIrq *pObj, int id, int k, int hk, std::string name) :
	kernel_size (k),
	hist_kernel_size (hk),
	stats("output", Write) {

		// pObj, name, id and kernel_lines are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);

		for(int i = 0; i < BINS_NO; i++)
			hist_bin[i] = 0;

		for(int i = 0; i < MAX_PATCH_NO*MAX_PATCH_NO*4; i++) {
			patch_accum[i] = 0;
		}

		for(int i = 0; i < 16; i++) {
			accum_active_plane[i] = false;
			 hist_active_plane[i] = false;
		}

		hist_max_pad = hk >> 1;

		Reset();
}

void RawFilt::Reset(void) {
	for(int i = 0; i < MAX_PATCH_NO*4; i++) {
		accum_started [i] = false;
		still_to_accum[i] = false;
	}
	
	enable_hist = enable_accum = false;
	row_of_patches = 0;
}

void RawFilt::SetStatsWidth(int val, int reg)  {
	raw_params[reg].stats_width = val;
}

void RawFilt::SetRawFormat(int val, int reg) {
	raw_params[reg].raw_format = val;
}

void RawFilt::SetBayerPattern(int val, int reg) {
	if(raw_params[reg].raw_format == BAYER)
		raw_params[reg].bayer_pattern = val;
	else
		raw_params[reg].bayer_pattern = 0;
}

void RawFilt::SetGrGbImbEn(int val, int reg) {
	raw_params[reg].grgb_imb_en = val;
}

void RawFilt::SetHotPixEn(int val, int reg) {
	raw_params[reg].hot_pix_en = val;
}

void RawFilt::SetHotGreenPixEn(int val, int reg) {
	raw_params[reg].hot_green_pix_en = val;
}

void RawFilt::SetStatsPatchEn(int val, int reg) {
	raw_params[reg].stats_patch_en = val;
}

void RawFilt::SetStatsHistEn(int val, int reg) {
	raw_params[reg].stats_hist_en = val;
}

void RawFilt::SetDataWidth(int val, int reg) {
	raw_params[reg].data_width = val;
	// Mask the GrGb Imbalance bits of interest
	promotion_used_bits = 16;
	promotion_shift_amount = promotion_used_bits - (raw_params[reg].data_width + 1);
	// Mask the Bad Pixel Detection bits of interest
	bad_pixel_used_bits = 8;
	bad_pixel_shift_amount = promotion_used_bits - bad_pixel_used_bits;
	bad_pixel_mask = ((1 << bad_pixel_used_bits) - 1) << bad_pixel_shift_amount;
	// Mask the histogram bits of interest
	hist_used_bits = GetMsb(BINS_NO);
	hist_shift_amount = (raw_params[reg].data_width + 1) - hist_used_bits;
	hist_mask = ((1 << hist_used_bits) - 1) << hist_shift_amount;
	// Mask the patch accumulation bits of interest
	accum_used_bits = 8;
	if(raw_params[reg].data_width + 1 >= 8) {
		accum_shift_amount = (raw_params[reg].data_width + 1) - accum_used_bits;
		accum_mask = ((1 << accum_used_bits)  - 1) << accum_shift_amount;
	} else {
		accum_shift_amount = 0;
		accum_mask = ((1 << (raw_params[reg].data_width + 1)) - 1) << accum_shift_amount;
	}
}

void RawFilt::SetBadPixelThr(int val, int reg) {
	raw_params[reg].bad_pixel_thr = val;
}

void RawFilt::SetDarkPlato(int val, int reg) {
	raw_params[reg].plato_dark = val;
}

void RawFilt::SetBrightPlato(int val, int reg) {
	raw_params[reg].plato_bright = val;
}

void RawFilt::SetDarkSlope(int val, int reg) {
	raw_params[reg].slope_dark = val;
}

void RawFilt::SetBrightSlope(int val, int reg) {
	raw_params[reg].slope_bright = val;
}

void RawFilt::SetAlphaRBCold(int val, int reg) {
	raw_params[reg].alpha_rb_cold = val;
}

void RawFilt::SetAlphaRBHot(int val, int reg) {
	raw_params[reg].alpha_rb_hot = val;
}

void RawFilt::SetAlphaGCold(int val, int reg) {
	raw_params[reg].alpha_g_cold = val;
}

void RawFilt::SetAlphaGHot(int val, int reg) {
	raw_params[reg].alpha_g_hot = val;
}

void RawFilt::SetNoiseLevel(int val, int reg) {
	raw_params[reg].noise_level = val;
}

void RawFilt::SetGain0(int val, int reg) {
	raw_params[reg].gain0.full = val;
}

void RawFilt::SetGain1(int val, int reg) {
	raw_params[reg].gain1.full = val;
}

void RawFilt::SetGain2(int val, int reg) {
	raw_params[reg].gain2.full = val;
}

void RawFilt::SetGain3(int val, int reg) {
	raw_params[reg].gain3.full = val;
}

void RawFilt::SetSat0(int val, int reg) {
	raw_params[reg].sat0 = val;
}

void RawFilt::SetSat1(int val, int reg) {
	raw_params[reg].sat1 = val;
}

void RawFilt::SetSat2(int val, int reg) {
	raw_params[reg].sat2 = val;
}

void RawFilt::SetSat3(int val, int reg) {
	raw_params[reg].sat3 = val;
}

void RawFilt::SetNumberOfPatchesH(int val, int reg) {
	if(val > MAX_PATCH_NO - 1) {
		std::cerr << "Invalid number of horizontal patches: must be less than " << MAX_PATCH_NO << std::endl;
		abort();
	}
	raw_params[reg].h_patches = val;
}

void RawFilt::SetNumberOfPatchesV(int val, int reg) {
	if(val > MAX_PATCH_NO - 1) {
		std::cerr << "Invalid number of vertical patches: must be less than " << MAX_PATCH_NO << std::endl;
		abort();
	}
	raw_params[reg].v_patches = val;
}

void RawFilt::SetPatchWidth(int val, int reg) {
	if(val > MAX_PATCH_SIZE - 1) {
		std::cerr << "Invalid patch width: must be less than " << MAX_PATCH_SIZE << std::endl;
		abort();
	}
	if(raw_params[reg].raw_format == BAYER && (val + 1) % 2) {
		std::cerr << "Invalid patch width: must be an even value in Bayer mode." << std::endl;
		abort();
	}
	raw_params[reg].patch_width = val;
}

void RawFilt::SetPatchHeight(int val, int reg) {
	if(val > MAX_PATCH_SIZE - 1) {
		std::cerr << "Invalid patch height: must be less than " << MAX_PATCH_SIZE << std::endl;
		abort();
	}
	if(raw_params[reg].raw_format == BAYER && (val + 1) % 2) {
		std::cerr << "Invalid patch height: must be an even value in Bayer mode." << std::endl;
		abort();
	}
	raw_params[reg].patch_height = val;
}

void RawFilt::SetStartX(int val, int reg) {
	if(val >= base_params[reg].width) {
		std::cerr << "Invalid X starting point: must be less than " << base_params[reg].width << std::endl;
		abort();
	}
	raw_params[reg].x_start = val;
}

void RawFilt::SetStartY(int val, int reg) {
	if(val >= base_params[reg].height) {
		std::cerr << "Invalid Y starting point: must be less than " << base_params[reg].height << std::endl;
		abort();
	}
	raw_params[reg].y_start = val;
}

void RawFilt::SetSkipX(int val, int reg) {
	if(val < raw_params[reg].patch_width || val > base_params[reg].width - 1) {
		std::cerr << "Invalid X skip: must be in range [" << raw_params[reg].patch_width << "," << base_params[reg].width << ")" << std::endl;
		abort();
	}
	if(raw_params[reg].x_start + raw_params[reg].h_patches * val + raw_params[reg].patch_width > base_params[reg].width) {
		std::cerr << "Invalid configuration: exceeds width boundaries!" << std::endl;
		abort();
	}
	raw_params[reg].x_skip = val;
}

void RawFilt::SetSkipY(int val, int reg) {
	if(val < raw_params[reg].patch_height || val > base_params[reg].height - 1) {
		std::cerr << "Invalid Y skip: must be in range [" << raw_params[reg].patch_height << "," << base_params[reg].height << ")" << std::endl;
		abort();
	}
	if(raw_params[reg].y_start + raw_params[reg].v_patches * val + raw_params[reg].patch_height > base_params[reg].height) {
		std::cerr << "Invalid configuration: exceeds height boundaries!" << std::endl;
		abort();
	}
	raw_params[reg].y_skip = val;
}

void RawFilt::SetActivePlanes(int val, int reg) {
	int cnt = raw_params[reg].active_planes_no + 1;
	raw_params[reg].active_planes = val;
	if(raw_format == BAYER) {
		accum_active_plane[(raw_params[reg].active_planes >>  0) & 0xf] = true;
	} else {
		for(int shift = 0; cnt; cnt--, shift += 4)
			accum_active_plane[(raw_params[reg].active_planes >> shift) & 0xf] = true;
	}
	hist_active_plane[(raw_params[reg].active_planes >> 16) & 0xf] = true;
}

void RawFilt::SetActivePlanesNo(int val, int reg) {
	raw_params[reg].active_planes_no = val;
}

void RawFilt::DetMinMax(uint16_t *kernel) {
	uint16_t *green_kernel;
	int grk_size = ((kernel_lines*kernel_lines) >> 1) + 1;

	green_kernel = new uint16_t[grk_size];
	for(int gr = 0, grk = 0; gr < kernel_lines*kernel_lines; gr += 2, grk++)
		green_kernel[grk] = (kernel[gr] & bad_pixel_mask) >> bad_pixel_shift_amount;

	qsort(green_kernel, grk_size, sizeof(*green_kernel), icompare);

	min1 = green_kernel[0];
	min2 = green_kernel[1];
	max1 = green_kernel[grk_size - 2];
	max2 = green_kernel[grk_size - 1];

	delete[] green_kernel;
}

void RawFilt::PromoteTo16Bits(uint16_t **src, uint16_t **src_16) {
	for(int l = 0; l < kernel_size; l++)
		for(int pix = 0; pix < width; pix++)
			src_16[l][pix] = src[l][pix] << promotion_shift_amount;
}

uint16_t RawFilt::HpsAdjustment(uint16_t *kernel, int x) {
	uint32_t energy_h, energy_v, energy_f, energy_b;
	int32_t emin;
	int thresh_min, thresh_max;
	uint16_t min1, min2, max1, max2;
	DIR min_dir;

	// Energy metric in horizontal direction
	energy_h = ((abs(kernel[7] - kernel[5]) + abs(kernel[ 9] - kernel[ 7]) + abs(kernel[17] - kernel[15]) + abs(kernel[19] - kernel[17]))*3 + 
	             abs(kernel[8] - kernel[6]) + abs(kernel[18] - kernel[16]) + abs(kernel[14] - kernel[10])) >> 4;

	// Energy metric in vertical direction
	energy_v = ((abs(kernel[11] - kernel[1]) + abs(kernel[21] - kernel[11]) + abs(kernel[13] - kernel[3]) + abs(kernel[23] - kernel[13]))*3 + 
	             abs(kernel[16] - kernel[6]) + abs(kernel[18] - kernel[ 8]) + abs(kernel[22] - kernel[2])) >> 4;

	// Energy metric in forward diagonal direction
	energy_f = ((abs(kernel[3] - kernel[11]) + abs(kernel[ 7] - kernel[15]) + abs(kernel[9] - kernel[17]) +  abs(kernel[13] - kernel[21]))*3 + 
	             abs(kernel[2] - kernel[10]) + abs(kernel[14] - kernel[22]) + abs(kernel[8] - kernel[16]) + (abs(kernel[ 4] - kernel[20]) >> 1)) >> 4;

	// Energy metric in backward diagonal direction
	energy_b = ((abs(kernel[1] - kernel[13]) + abs(kernel[ 7] - kernel[19]) + abs(kernel[5] - kernel[17]) +  abs(kernel[11] - kernel[23]))*3 + 
	             abs(kernel[2] - kernel[14]) + abs(kernel[10] - kernel[22]) + abs(kernel[6] - kernel[18]) + (abs(kernel[ 0] - kernel[24]) >> 1)) >> 4;

	emin = MIN(MIN(energy_h, energy_v), MIN(energy_f, energy_b));
	min_dir = (emin == energy_b) ? BDIAG : (emin == energy_f) ? FDIAG : (emin == energy_v) ? VERT : HOR;
	emin = MAX(0, emin - noise_level);

	switch(min_dir) {
	case HOR :
		max1 = MAX(MAX(kernel[ 8], kernel[16]), MAX(kernel[6], kernel[18]));
		max2 =     MAX(kernel[10], kernel[14]);
		min1 = MIN(MIN(kernel[ 8], kernel[16]), MIN(kernel[6], kernel[18]));
		min2 =     MIN(kernel[10], kernel[14]);
		break;
	case VERT :
		max1 = MAX(MAX(kernel[ 8], kernel[16]), MAX(kernel[6], kernel[18]));
		max2 =     MAX(kernel[ 2], kernel[22]);
		min1 = MIN(MIN(kernel[ 8], kernel[16]), MIN(kernel[6], kernel[18]));
		min2 =     MIN(kernel[ 2], kernel[22]);
		break;
	case FDIAG :
		max1 =     MAX(kernel[ 8], kernel[16]);
		max2 =     MAX(kernel[ 4], kernel[20]);
		min1 =     MIN(kernel[ 8], kernel[16]);
		min2 =     MIN(kernel[ 4], kernel[20]);
		break;
	case BDIAG :
		max1 =     MAX(kernel[ 6], kernel[18]);
		max2 =     MAX(kernel[ 0], kernel[24]);
		min1 =     MIN(kernel[ 6], kernel[18]);
		min2 =     MIN(kernel[ 0], kernel[24]);
		break;
	}

	if(BP(bayer_pattern, MID, x) || RP(bayer_pattern, MID, x)) {
		thresh_max = max2 + emin * alpha_rb_hot;
		thresh_min = min2 - emin * alpha_rb_cold;
	} else {
		thresh_max = MAX(max1,max2) + emin * alpha_g_hot;
		thresh_min = MIN(min1,min2) - emin * alpha_g_cold;
	}

	if(kernel[KERNEL_CENTRE] > thresh_max)
		return (uint16_t)MAX(max2, 2*thresh_max - kernel[KERNEL_CENTRE]);
	else if(kernel[KERNEL_CENTRE] < thresh_min)
		return (uint16_t)MIN(min2, 2*thresh_min - kernel[KERNEL_CENTRE]);
	else
		return kernel[KERNEL_CENTRE];
}

void RawFilt::buildHistKernel(uint16_t **src, uint16_t *kernel, int x) {
	// Horizontal padding
	int pad_pixels;

	if(x < hist_max_pad)
		pad_pixels = hist_max_pad - x;
	else if(x > width - hist_max_pad - 1)
		pad_pixels = (width - 1) - (x + hist_max_pad);
	else
		pad_pixels = 0;

	if (pad_pixels > 0)
		// Padding at left
		HistPadAtLeft(src, kernel, pad_pixels, x);
	else if (pad_pixels < 0)
		// Padding at right
		HistPadAtRight(src, kernel, -pad_pixels, x);
	else
		HistMiddleExecutionH(src, kernel, x);
}

void RawFilt::HistPadAtLeft(uint16_t **src, uint16_t *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the first pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < hist_kernel_size; kv++)
		for (kh = 0; kh < pad_pixels; kh++)
			kernel[kv*hist_kernel_size + kh] = src[kv][0];

	for (kv = 0; kv < hist_kernel_size; kv++)
		for (kh = pad_pixels; kh < hist_kernel_size; kh++)
			kernel[kv*hist_kernel_size + kh] = src[kv][crt_pixel + kh - hist_max_pad];
}

void RawFilt::HistPadAtRight(uint16_t **src, uint16_t *kernel, int pad_pixels, int crt_pixel) {
	// Replicate the last pixel of the current line by a number of times
	// based on the kernel size and the kernel centre (crt_pixel)
	int kh;
	int kv;

	for (kv = 0; kv < hist_kernel_size; kv++)
		for (kh = 0; kh < hist_kernel_size - pad_pixels; kh++)
			kernel[kv*hist_kernel_size + kh] = src[kv][crt_pixel + kh - hist_max_pad];

	for (kv = 0; kv < hist_kernel_size; kv++)
		for (kh = hist_kernel_size - pad_pixels; kh < hist_kernel_size; kh++)
			kernel[kv*hist_kernel_size + kh] = kernel[kv*hist_kernel_size + kh - 1];
}

void RawFilt::HistMiddleExecutionH(uint16_t **src, uint16_t *kernel, int crt_pixel) {
	// No padding needed since we have the necessary pixels for the
	// respective kernel size in order to continue the execution
	int kh;
	int kv;

	for (kv = 0; kv < hist_kernel_size; kv++)
		for (kh = 0; kh < hist_kernel_size; kh++)
			kernel[kv*hist_kernel_size + kh] = src[kv][crt_pixel + kh - hist_max_pad];
}

void RawFilt::setHistOutputLine(void) {
	stats_ptr = (uint32_t *)stats.buffer + stats.GetOffset() 
	    + (stats.GetLineStride()  / stats.GetFormat()) * stats.GetBufferIdxNoWrap(height)
	    +  stats_width - BINS_NO;
}

void RawFilt::setStatsOutputLine(void) {
	stats_ptr = (uint32_t *)stats.buffer + stats.GetOffset()
	    + (stats.GetLineStride()  / stats.GetFormat()) * stats.GetBufferIdxNoWrap(height);
}

void RawFilt::BayerAccumulation(uint16_t **src) {
	int hp_cnt, crt_ch, accums_per_line;

	if(line_idx == y_start + (y_skip + 1) * row_of_patches) {
		accum_started[row_of_patches] = true;
		for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++)
			for(vp_cnt[row_of_patches] = 0; vp_cnt[row_of_patches] < patch_height + 1 && vp_cnt[row_of_patches] < MID + 1; vp_cnt[row_of_patches]++)
				for(hp_cnt = x_start, accums_per_line = 0; hp_cnt < x_start + patch_width + 1, accums_per_line < 2; hp_cnt++, accums_per_line++)
					// Accumulate over the channels
					for(crt_ch = hp_cnt; crt_ch < x_start + patch_width + 1; crt_ch += 2)
							patch_accum[(row_of_patches * (h_patches + 1) + col_of_patches) * 4 + (vp_cnt[row_of_patches] & 0x1) * 2 + accums_per_line] += 
																(src[vp_cnt[row_of_patches] + MID][crt_ch + (x_skip + 1) * col_of_patches] & accum_mask) >> accum_shift_amount;
	}
	// Accumulate what wasn't encompassed in the initial kernel
	if(still_to_accum[row_of_patches]) {
		for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++)
			for(hp_cnt = x_start, accums_per_line = 0; hp_cnt < x_start + patch_width + 1, accums_per_line < 2; hp_cnt++, accums_per_line++)
				// Accumulate over the channels
				for(crt_ch = hp_cnt; crt_ch < x_start + patch_width + 1; crt_ch += 2)
						patch_accum[(row_of_patches * (h_patches + 1) + col_of_patches) * 4 + (vp_cnt[row_of_patches] & 0x1) * 2 + accums_per_line] += 
															(src[kernel_lines - 1][crt_ch + (x_skip + 1) * col_of_patches] & accum_mask) >> accum_shift_amount;
		vp_cnt[row_of_patches]++;
	}
	// Determine if there's still a need to accumulate for ulterior runs
	if(accum_started[row_of_patches])
		if(vp_cnt[row_of_patches] < patch_height + 1) {
			still_to_accum[row_of_patches] = true;
		} else {
			still_to_accum[row_of_patches] = false;
			// Accumulation for a row of patches finished, save results to output buffer and reset the RAM value
			for(int lines = 0; lines < 2; lines++)
				for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++)
					for(accums_per_line = 0; accums_per_line < 2; accums_per_line++) {
						*stats_ptr++ = patch_accum[(row_of_patches * (h_patches + 1) + col_of_patches) * 4 + lines * 2 + accums_per_line];
						patch_accum[(row_of_patches * (h_patches + 1) + col_of_patches) * 4 + lines * 2 + accums_per_line] = 0;
					}
			row_of_patches++;
		}
}

void RawFilt::PlanarAccumulation(uint16_t **src) {
	int hp_cnt;

	if(line_idx == y_start + (y_skip + 1) * row_of_patches) {
		accum_started[row_of_patches * (active_planes_no + 1) + active_planes_cnt] = true;
		for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++)
			for(vp_cnt[row_of_patches * (active_planes_no + 1) + active_planes_cnt] = 0;       vp_cnt[row_of_patches *(active_planes_no + 1) + active_planes_cnt] < patch_height + 1 
			 && vp_cnt[row_of_patches * (active_planes_no + 1) + active_planes_cnt] < MID + 1; vp_cnt[row_of_patches *(active_planes_no + 1) + active_planes_cnt]++)
				for(hp_cnt = x_start; hp_cnt < x_start + patch_width + 1; hp_cnt++)
					// Accumulate over the current plane
					patch_accum[(row_of_patches * (active_planes_no + 1) + active_planes_cnt) * (h_patches + 1) + col_of_patches] += 
						(src[vp_cnt[row_of_patches * (active_planes_no + 1) + active_planes_cnt] + 2][hp_cnt + (x_skip + 1) * col_of_patches] & accum_mask) >> accum_shift_amount;
	}
	// Accumulate what wasn't encompassed in the initial kernel
	if(still_to_accum[row_of_patches * (active_planes_no + 1) + active_planes_cnt]) {
		for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++)
			for(hp_cnt = x_start; hp_cnt < x_start + patch_width + 1; hp_cnt++)
				// Accumulate over the current plane
				patch_accum[(row_of_patches * (active_planes_no + 1) + active_planes_cnt) * (h_patches + 1) + col_of_patches] += 
						(src[kernel_lines - 1][hp_cnt + (x_skip + 1) * col_of_patches] & accum_mask) >> accum_shift_amount;
		vp_cnt[row_of_patches * (active_planes_no + 1) + active_planes_cnt]++;
	}
	// Determine if there's still a need to accumulate for ulterior runs
	if(accum_started[row_of_patches * (active_planes_no + 1) + active_planes_cnt])
		if(vp_cnt[row_of_patches * (active_planes_no + 1) + active_planes_cnt] < patch_height + 1) {
			still_to_accum[row_of_patches * (active_planes_no + 1) + active_planes_cnt] = true;
		} else {
			still_to_accum[row_of_patches * (active_planes_no + 1) + active_planes_cnt] = false;
			// Accumulation for a row of patches finished, save results to output buffer and reset the RAM value
			for(col_of_patches = 0; col_of_patches < h_patches + 1; col_of_patches++) {
				*stats_ptr++ = patch_accum[(row_of_patches * (active_planes_no + 1) + active_planes_cnt) * (h_patches + 1) + col_of_patches];
				patch_accum[(row_of_patches * (active_planes_no + 1) + active_planes_cnt) * (h_patches + 1) + col_of_patches] = 0;
			}
			if(active_planes_cnt == active_planes_no)
				row_of_patches++;
		}
}

void RawFilt::BayerHistogram(uint16_t **src) {
	// Allocating histogram kernel
	uint16_t *hist_kernel = new uint16_t[hist_kernel_size*hist_kernel_size];
	uint32_t converted_y;

	int x = 0;
	while(x < width) {
		// Skip padded kernels
		if(x < (hist_kernel_size >> 1) || x > (width - 1) - (hist_kernel_size >> 1)) {
			x++;
			continue;
		}

		// Reset the current Y value
		converted_y = 0;

		// Build the 2D kernel
		buildHistKernel(src, hist_kernel, x);

		// Convert to luma
		for(int r = 0; r < hist_kernel_size; r++)
			for(int c = 0; c < hist_kernel_size; c++)
				converted_y += hist_kernel[r * hist_kernel_size + c] * luma_kernel[r * hist_kernel_size + c];

		// Scale and mask the bits of interest
		converted_y >>= 4;
		converted_y = (converted_y & hist_mask) >> hist_shift_amount;

		// Update the histogram bins
		hist_bin[converted_y]++;

		x++;
	}
}

void RawFilt::PlanarHistogram(uint16_t *src) {
	uint16_t luma_pix;

	int x = 0;
	while(x < width) {
		// Mask the bits of interest
		luma_pix = (src[x] & hist_mask) >> hist_shift_amount;

		// Update the histogram bins
		hist_bin[luma_pix]++;

		x++;
	}
}

void RawFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	stats.SelectBufferParameters();
	stats_width      = raw_params[reg].stats_width;
	raw_format       = raw_params[reg].raw_format;
	bayer_pattern    = raw_params[reg].bayer_pattern;
	grgb_imb_en      = raw_params[reg].grgb_imb_en;
	hot_pix_en       = raw_params[reg].hot_pix_en;
	hot_green_pix_en = raw_params[reg].hot_green_pix_en;
	stats_patch_en   = raw_params[reg].stats_patch_en;
	stats_hist_en    = raw_params[reg].stats_hist_en;
	data_width       = raw_params[reg].data_width;
	bad_pixel_thr    = raw_params[reg].bad_pixel_thr;
	plato_dark       = raw_params[reg].plato_dark;
	plato_bright     = raw_params[reg].plato_bright;
	slope_dark       = raw_params[reg].slope_dark;
	slope_bright     = raw_params[reg].slope_bright;
	alpha_rb_cold    = raw_params[reg].alpha_rb_cold;
	alpha_rb_hot     = raw_params[reg].alpha_rb_hot;
	alpha_g_cold     = raw_params[reg].alpha_g_cold;
	alpha_g_hot      = raw_params[reg].alpha_g_hot;
	noise_level      = raw_params[reg].noise_level;
	gain0            = raw_params[reg].gain0;
	gain1            = raw_params[reg].gain1;
	gain2            = raw_params[reg].gain2;
	gain3            = raw_params[reg].gain3;
	sat0             = raw_params[reg].sat0;
	sat1             = raw_params[reg].sat1;
	sat2             = raw_params[reg].sat2;
	sat3             = raw_params[reg].sat3;
	h_patches        = raw_params[reg].h_patches;
	v_patches        = raw_params[reg].v_patches;
	patch_width      = raw_params[reg].patch_width;
	patch_height     = raw_params[reg].patch_height;
	x_start          = raw_params[reg].x_start;
	y_start          = raw_params[reg].y_start;
	x_skip           = raw_params[reg].x_skip;
	y_skip           = raw_params[reg].y_skip;
	active_planes    = raw_params[reg].active_planes;
	active_planes_no = raw_params[reg].active_planes_no;
}

void RawFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint8_t  *      out_ptr_8  = 0;
	uint16_t *      out_ptr_16 = 0;

	void **in_ptr;
	void *out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  *[kernel_lines];
		      in_ptr_8  = new uint8_t  *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[kernel_lines];
		      in_ptr_16 = new uint16_t *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Set the stats pointers at the start of the frame
	if(line_idx == 0 && (stats_hist_en || stats_patch_en))
		setStatsOutputLine();

	// Filter a line from each plane
	active_planes_cnt = 0;
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Prepare filter buffer line pointers then run
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_8 , kernel_lines, pl);
			PackInputLines(split_in_ptr_8 , kernel_lines, in_ptr_8 );
		} else {
			getKernelLines(split_in_ptr_16, kernel_lines, pl);
			PackInputLines(split_in_ptr_16, kernel_lines, in_ptr_16);
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			setOutputLine(&out_ptr_8 , pl);
			out_ptr = (void *)out_ptr_8 ;
		} else {
			setOutputLine(&out_ptr_16, pl);
			out_ptr = (void *)out_ptr_16;
		}
		if(stats_patch_en)
			if(accum_active_plane[pl] && row_of_patches != v_patches + 1)
				enable_accum = true;
			else
				enable_accum = false;
		if(stats_hist_en)
			if(hist_active_plane[pl])
				enable_hist = true;
			else
				enable_hist = false;
		
		Run(in_ptr, out_ptr);
		if(accum_active_plane[pl])
			active_planes_cnt++;
	}
	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// At the end of the frame
	if (EndOfFrame()) {
		// Trigger end of frame interrupt
		EndOfFrameIrq();
		if(stats_hist_en || stats_patch_en) {
			// Save the histogram to CMX and reset the bins
			if(stats_hist_en) {
				setHistOutputLine();
				for(int bin = 0; bin < BINS_NO; bin++) {
					*stats_ptr++ = hist_bin[bin];
					hist_bin[bin] = 0;
				}
			}
			// Advance in the statistics output buffer for the next frame
			if(stats.GetBuffLines() == 0)
				stats.IncBufferIdxNoWrap(MAX_FRAMES_NO);
			else
				stats.IncBufferIdx();
			Reset();

			// Trigger gathering statistics interrupt
			SetId(SIPP_STATS_ID);
			EndOfFrameIrq();

			// Restore the ID for the next frame
			SetId(SIPP_RAW_ID);
		}
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;
	delete[] in_ptr;
}

void RawFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **split_in_ptr_8  = 0;
	uint8_t  **      in_ptr_8  = 0;
	uint16_t **split_in_ptr_16 = 0;
	uint16_t **      in_ptr_16 = 0;
	uint8_t  *      out_ptr_8  = 0;
	uint16_t *      out_ptr_16 = 0;

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
		split_in_ptr_8  = new uint8_t  *[kernel_lines];
		      in_ptr_8  = new uint8_t  *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[kernel_lines];
		      in_ptr_16 = new uint16_t *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_lines)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Set the stats pointers at the start of the frame
		if(line_idx == 0 && (stats_hist_en || stats_patch_en))
			setStatsOutputLine();

		// Filter a line from each plane
		active_planes_cnt = 0;
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Prepare filter buffer line pointers then run
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_in_ptr_8 , kernel_lines, pl);
				PackInputLines(split_in_ptr_8 , kernel_lines, in_ptr_8 );
			} else {
				getKernelLines(split_in_ptr_16, kernel_lines, pl);
				PackInputLines(split_in_ptr_16, kernel_lines, in_ptr_16);
			}
			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				setOutputLine(&out_ptr_8 , pl);
				out_ptr = (void *)out_ptr_8 ;
			} else {
				setOutputLine(&out_ptr_16, pl);
				out_ptr = (void *)out_ptr_16;
			}
			if(stats_patch_en)
				if(accum_active_plane[pl] && row_of_patches != v_patches + 1)
					enable_accum = true;
				else
					enable_accum = false;
			if(stats_hist_en)
				if(hist_active_plane[pl])
					enable_hist = true;
				else
					enable_hist = false;

			Run(in_ptr, out_ptr);
			if(accum_active_plane[pl])
				active_planes_cnt++;
		}
		// Update buffer fill levels and input frame line index
		UpdateBuffers();
		IncLineIdx();

		// At the end of the frame
		if (EndOfFrame()) {
			// Trigger end of frame interrupt
			EndOfFrameIrq();
			if(stats_hist_en || stats_patch_en) {
				// Save the histogram to CMX and reset the bins
				if(stats_hist_en) {
					setHistOutputLine();
					for(int bin = 0; bin < BINS_NO; bin++) {
						*stats_ptr++ = hist_bin[bin];
						hist_bin[bin] = 0;
					}
				}
				// Advance in the statistics output buffer for the next frame
				if(stats.GetBuffLines() == 0)
					stats.IncBufferIdxNoWrap(MAX_FRAMES_NO);
				else
					stats.IncBufferIdx();
				Reset();

				// Trigger gathering statistics interrupt
				SetId(SIPP_STATS_ID);
				EndOfFrameIrq();

				// Restore the ID for the next frame
				SetId(SIPP_RAW_ID);
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
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;
	delete[] in_ptr;
}

void *RawFilt::Run(void **in_ptr, void *out_ptr) {
	uint8_t  **      src_8    = 0;
	uint16_t **      src_16   = 0;
	uint16_t **      src_prom = 0;
	uint8_t  *       dst_8    = 0;
	uint8_t  *packed_dst_8    = 0;
	uint16_t *       dst_16   = 0;
	uint16_t *packed_dst_16   = 0;

	uint16_t *kernel;
	uint16_t *grgb_result;
	uint16_t *hotpix_result;
	
	// Allocating array of pointers to input lines
	// and setting input pointers
	src_16 = new uint16_t *[kernel_lines];
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_8  = new uint8_t  *[kernel_lines];
		// Setting input pointers
		for (int i = 0; i < kernel_lines; i++) {
			src_8 [i] = (uint8_t  *)in_ptr[i];
		}
		// Cast u8 to u16
		for (int i = 0; i < kernel_size; i++) {
			src_16[i] = new uint16_t[width];
			convertKernel(src_8[i], src_16[i], width);
		}
	} else {
		src_16 = new uint16_t *[kernel_lines];
		// Setting input pointers
		for (int i = 0; i < kernel_lines; i++) {
			src_16[i] = (uint16_t *)in_ptr[i];
		}
	}

	// Allocating the internal kernel for GrGb imbalance
	kernel = new uint16_t[kernel_lines * kernel_lines];
	
	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_8  = (uint8_t  *)out_ptr;
		packed_dst_8  = new uint8_t [width];
	} else {
		       dst_16 = (uint16_t *)out_ptr;
		packed_dst_16 = new uint16_t[width];
	}

	uint16_t G_dn;
	uint16_t inG;
	uint16_t luma;
	uint16_t G_smooth;
	int full_gd;
	int plato;
	int slope;
	uint16_t adjusted;
	uint16_t before_gain;
	FIXED16_8 filtered_val;
	FIXED24_8 after_gain;

	int x = 0;

	// Histogram statistics
	if(enable_hist)
		if(raw_format == BAYER) {
			if(line_idx >= MID)
				BayerHistogram(src_16);
		} else {
			PlanarHistogram(src_16[MID]);
		}
	// Patch statistics
	x = 0;
	if(enable_accum && row_of_patches < v_patches + 1)
		if(raw_format == BAYER)
			BayerAccumulation(src_16);
		else
			PlanarAccumulation(src_16);

	// Apply gains and saturation on the first/last two lines
	x = 0;
	if(line_idx < MID || line_idx > (height - 1) - MID) {
		while(x < width) {
			// Convert the original green value to U(16,8)
			filtered_val.full = FLOAT_TO_FIXED16_8(src_16[MID][x], 0.0f);

			// Apply the corresponding digital gain
			switch(x % 4) {
			case 0 :
				after_gain.full = MUL8_8(filtered_val, gain0);
				break;
			case 1 :
				after_gain.full = MUL8_8(filtered_val, gain1);
				break;
			case 2 :
				after_gain.full = MUL8_8(filtered_val, gain2);
				break;
			case 3 :
				after_gain.full = MUL8_8(filtered_val, gain3);
				break;
			default:
				;
			}

			// Apply the corresponding saturation
			switch(x % 4) {
			case 0 :
				after_gain.part.integer = (after_gain.part.integer > sat0) ? sat0 : after_gain.part.integer;
				break;
			case 1 :
				after_gain.part.integer = (after_gain.part.integer > sat1) ? sat1 : after_gain.part.integer;
				break;
			case 2 :
				after_gain.part.integer = (after_gain.part.integer > sat2) ? sat2 : after_gain.part.integer;
				break;
			case 3 :
				after_gain.part.integer = (after_gain.part.integer > sat3) ? sat3 : after_gain.part.integer;
				break;
			default:
				;
			}

			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				packed_dst_8 [x] = (uint8_t )(after_gain.part.integer);
			else
				packed_dst_16[x] = (uint16_t)(after_gain.part.integer);
			x++;
		}

		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			SplitOutputLine(packed_dst_8 , width, dst_8 );
			delete[] packed_dst_8 ;
		} else {
			SplitOutputLine(packed_dst_16, width, dst_16);
			delete[] packed_dst_16;
		}

		return out_ptr;
	}

	// Promote incoming data to 16 bits
	if(grgb_imb_en || hot_pix_en) {
		src_prom = new uint16_t *[kernel_lines];
		for(int l = 0; l < kernel_lines; l++)
			src_prom[l] = new uint16_t[width];
		PromoteTo16Bits(src_16, src_prom);
	}

	// Gr/Gb imbalance
	if(grgb_imb_en) {
		grgb_result = new uint16_t[width];
		while(x < width) {
			// Copy the first/last two pixels and the red/blue pixels
			if(              x < MID     || x > (width - 1) - MID     ||
			   BP(bayer_pattern, MID, x) || RP(bayer_pattern, MID, x)) {
				grgb_result[x] = src_16[MID][x];
				x++;
				continue;
			}

			// Build the 2D kernel
			buildKernel(src_prom, kernel, kernel_lines, x);

			// Get min/max and second-min/max of all green pixels in 5x5 area
			DetMinMax(kernel);

			// If an outlier (hot or cold pixel) was detected, don't proceed to GrGb imbalance
			if(min2 - min1 > bad_pixel_thr || max2 - max1 > bad_pixel_thr) {
				grgb_result[x] = kernel[KERNEL_CENTRE] >> promotion_shift_amount;
				x++;
				continue;
			}

			// Get the green centre
 			inG = kernel[KERNEL_CENTRE];

			// Sum the 4 diagonal neighbours
			G_dn = (kernel[ 6] + kernel[ 8]
			                   +
			        kernel[16] + kernel[18]) >> 2;

			// Luma is the average of 5 green pixels (current and its neighbours)
			luma = (G_dn + inG) >> 1;

			// Run a smoothing filter over the all green pixels
			G_smooth = (kernel[ 0]*1 +  kernel[ 2]*2 +  kernel[ 4]*1 +
			            kernel[10]*2 +  inG       *4 +  kernel[14]*2 +
			            kernel[20]*1 +  kernel[22]*2 +  kernel[24]*1)  >> 4;

			// Compute the green differences
			full_gd = (G_dn - G_smooth) / 2;
			plato = (plato_dark << 2) + (((plato_bright - plato_dark)*luma) / 4096);
			slope = (slope_dark << 2) + (((slope_bright - slope_dark)*luma) / 4096);

			// Compute the full green difference
			full_gd =   full_gd                * ((full_gd<=plato  ) && (full_gd>=(-plato)))     
			          + plato                  * ((full_gd> plato  ) && (full_gd<=  slope))      
			          + (plato+slope-full_gd)  * ((full_gd>slope   ) && (full_gd<=(plato+slope)))
			          + (-plato)               * ((full_gd<(-plato)) && (full_gd>=(-slope)))     
			          + (-plato-slope-full_gd) * ((full_gd<(-slope)) && (full_gd>=(-slope-plato)));

			// Output the balanced green value
			int32_t out_val = (int32_t)(inG + full_gd) >> promotion_shift_amount;
			grgb_result[x] = (uint16_t)ClampWR((float)out_val, 0.0f, (float)((1 << (data_width + 1)) - 1));
			x++;
		}
		if(!hot_pix_en)
			delete[] src_prom;
	}

	// Hot pixel suppression
	x = 0;
	if(hot_pix_en) {
		hotpix_result = new uint16_t[width];
		if(grgb_imb_en)
			memcpy(hotpix_result, grgb_result, sizeof(*hotpix_result)*width);
		else
			memcpy(hotpix_result, src_16[MID], sizeof(*hotpix_result)*width);
		while(x < width) {
			// Copy the first/last two pixels if necessarry
			if(x < MID || x > (width - 1) - MID) {
				x++;
				continue;
			}

			// Suppress only hot/cold green pixels
			if(hot_green_pix_en && (BP(bayer_pattern, MID, x) || RP(bayer_pattern, MID, x))) {
				x++;
				continue;
			}

			// Build the 2D kernel
			buildKernel(src_prom, kernel, kernel_lines, x);

			// Suppress hot/cold pixels
			adjusted = HpsAdjustment(kernel, x);

			// Overwrite GrGb imbalance's result if the pixel is flagged as bad
			if(adjusted != kernel[KERNEL_CENTRE])
				hotpix_result[x] = adjusted >> promotion_shift_amount;
			x++;
		}
		delete[] src_prom;
	}

	// Digital gain and saturation
	x = 0;
	while(x < width) {
		// Convert either the original or the filtered green value to U(16,8)
		if(hot_pix_en)
			before_gain = hotpix_result[x];
		else if(grgb_imb_en)
			before_gain =   grgb_result[x];
		else
			before_gain =   src_16[MID][x];
		filtered_val.full = FLOAT_TO_FIXED16_8(before_gain, 0.0f);

		// Apply the corresponding digital gain
		switch(x % 4) {
		case 0 :
			after_gain.full = MUL8_8(filtered_val, gain0);
			break;
		case 1 :
			after_gain.full = MUL8_8(filtered_val, gain1);
			break;
		case 2 :
			after_gain.full = MUL8_8(filtered_val, gain2);
			break;
		case 3 :
			after_gain.full = MUL8_8(filtered_val, gain3);
			break;
		default:
			;
		}

		// Apply the corresponding saturation
		switch(x % 4) {
		case 0 :
			after_gain.part.integer = (after_gain.part.integer > sat0) ? sat0 : after_gain.part.integer;
			break;
		case 1 :
			after_gain.part.integer = (after_gain.part.integer > sat1) ? sat1 : after_gain.part.integer;
			break;
		case 2 :
			after_gain.part.integer = (after_gain.part.integer > sat2) ? sat2 : after_gain.part.integer;
			break;
		case 3 :
			after_gain.part.integer = (after_gain.part.integer > sat3) ? sat3 : after_gain.part.integer;
			break;
		default:
			;
		}

		// Output the value
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			packed_dst_8 [x] = (uint8_t )(after_gain.part.integer);
		else
			packed_dst_16[x] = (uint16_t)(after_gain.part.integer);
		x++;
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		SplitOutputLine(packed_dst_8 , width, dst_8 );
		delete[] packed_dst_8 ;
	} else {
		SplitOutputLine(packed_dst_16, width, dst_16);
		delete[] packed_dst_16;
	}

	delete[] kernel;
	if(grgb_imb_en)
		delete[] grgb_result;
	if(hot_pix_en)
		delete[] hotpix_result;

	delete[] src_16;
	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] src_8;

	return out_ptr;
}

// Comparison function for qsort()
int icompare(const void *a, const void *b) {
	uint16_t *pa = (uint16_t *)a;
	uint16_t *pb = (uint16_t *)b;
	return *pa == *pb ? 0 : *pa < *pb ? -1 : 1;
}
