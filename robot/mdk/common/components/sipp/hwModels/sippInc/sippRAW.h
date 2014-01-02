// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - RAW filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_RAW_H__
#define __SIPP_RAW_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE        (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

#define MID                  (kernel_size >> 1)

#define BINS_NO              64
#define HIST_SIZE            BINS_NO * 4

#define MAX_PATCH_NO         64
#define MAX_PATCH_SIZE       256
#define MAX_ACTIVE_PLANES_NO 4

#define MAX_FRAMES_NO        50 // This is passed as an argument to the IncBufferIdxNoWrap method, meaning that after 
                                // MAX_FRAMES_NO frames the stats buffer index will wrap-around

typedef enum {
	HOR = 0, VERT, FDIAG, BDIAG
} DIR;

const uint8_t luma_kernel[] = {1, 2, 1,
                               2, 4, 2,
                               1, 2, 1
};

typedef struct {
	int stats_width;
	int raw_format;
	int bayer_pattern;
	int grgb_imb_en;
	int hot_pix_en;
	int hot_green_pix_en;
	int stats_patch_en;
	int stats_hist_en;
	int data_width;
	int bad_pixel_thr;
	uint32_t plato_dark;
	uint32_t plato_bright;
	uint32_t slope_dark;
	uint32_t slope_bright;
	uint32_t alpha_rb_cold;
	uint32_t alpha_rb_hot;
	uint32_t alpha_g_cold;
	uint32_t alpha_g_hot;
	uint16_t noise_level;
	FIXED8_8 gain0;
	FIXED8_8 gain1;
	FIXED8_8 gain2;
	FIXED8_8 gain3;
	uint16_t sat0;
	uint16_t sat1;
	uint16_t sat2;
	uint16_t sat3;
	int h_patches;
	int v_patches;
	int patch_width;
	int patch_height;
	int x_start;
	int y_start;
	int x_skip;
	int y_skip;
	int active_planes;
	int active_planes_no;
} RawParameters;

class RawFilt : public SippBaseFilt {

public:
	Buffer stats;

	RawFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		int hk = 0,
		std::string name = "RAW filter");
	~RawFilt() {
	}

	void Reset(void);

	void SetStatsWidth(int, int reg = 0);
	void SetRawFormat(int, int reg = 0);
	void SetBayerPattern(int, int reg = 0);
	void SetGrGbImbEn(int, int reg = 0);
	void SetHotPixEn(int, int reg = 0);
	void SetHotGreenPixEn(int, int reg = 0);
	void SetStatsPatchEn(int, int reg = 0);
	void SetStatsHistEn(int, int reg = 0);
	void SetDataWidth(int, int reg = 0);
	void SetBadPixelThr(int, int reg = 0);

	void SetDarkPlato(int, int reg = 0);
	void SetBrightPlato(int, int reg = 0);
	void SetDarkSlope(int, int reg = 0);
	void SetBrightSlope(int, int reg = 0);

	void PromoteTo16Bits(uint16_t **, uint16_t **);
	uint16_t HpsAdjustment(uint16_t *, int);
	void DetMinMax(uint16_t *);

	void SetAlphaRBCold(int, int reg = 0);
	void SetAlphaRBHot(int, int reg = 0);
	void SetAlphaGCold(int, int reg = 0);
	void SetAlphaGHot(int, int reg = 0);
	void SetNoiseLevel(int, int reg = 0);

	void SetGain0(int, int reg = 0);
	void SetGain1(int, int reg = 0);
	void SetGain2(int, int reg = 0);
	void SetGain3(int, int reg = 0);
	void SetSat0(int, int reg = 0);
	void SetSat1(int, int reg = 0);
	void SetSat2(int, int reg = 0);
    void SetSat3(int, int reg = 0);

	void SetNumberOfPatchesH(int, int reg = 0);
	void SetNumberOfPatchesV(int, int reg = 0);
	void SetPatchWidth(int, int reg = 0);
	void SetPatchHeight(int, int reg = 0);
	void SetStartX(int, int reg = 0);
	void SetStartY(int, int reg = 0);
	void SetSkipX(int, int reg = 0);
	void SetSkipY(int, int reg = 0);

	void SetActivePlanes(int, int reg = 0);
	void SetActivePlanesNo(int, int reg = 0);

	void BayerAccumulation(uint16_t **);
	void PlanarAccumulation(uint16_t **);

	void BayerHistogram(uint16_t **);
	void PlanarHistogram(uint16_t *);

	int GetStatsWidth(int reg = 0)       { return raw_params[reg].stats_width; };
	int GetRawFormat(int reg = 0)        { return raw_params[reg].raw_format; };
	int GetBayerPattern(int reg = 0)     { return raw_params[reg].bayer_pattern; };
	int GetGrGbImbEn(int reg = 0)        { return raw_params[reg].grgb_imb_en; };
	int GetHotPixEn(int reg = 0)         { return raw_params[reg].hot_pix_en; };
	int GetHotGreenPixEn(int reg = 0)    { return raw_params[reg].hot_green_pix_en; };
	int GetStatsPatchEn(int reg = 0)     { return raw_params[reg].stats_patch_en; };
	int GetStatsHistEn(int reg = 0)      { return raw_params[reg].stats_hist_en; };
	int GetDataWidth(int reg = 0)        { return raw_params[reg].data_width; };
	int GetBadPixelThr(int reg = 0)      { return raw_params[reg].bad_pixel_thr; };
						      
	int GetDarkPlato(int reg = 0)        { return raw_params[reg].plato_dark; };
	int GetBrightPlato(int reg = 0)      { return raw_params[reg].plato_bright; };
	int GetDarkSlope(int reg = 0)        { return raw_params[reg].slope_dark; };
	int GetBrightSlope(int reg = 0)      { return raw_params[reg].slope_bright; };
						      
	int GetAlphaRBCold(int reg = 0)      { return raw_params[reg].alpha_rb_cold; };
	int GetAlphaRBHot(int reg = 0)       { return raw_params[reg].alpha_rb_hot; };
	int GetAlphaGCold(int reg = 0)       { return raw_params[reg].alpha_g_cold; };
	int GetAlphaGHot(int reg = 0)        { return raw_params[reg].alpha_g_hot; };
	int GetNoiseLevel(int reg = 0)       { return raw_params[reg].noise_level; };
						      
	int GetGain0(int reg = 0)            { return raw_params[reg].gain0.full; };
	int GetGain1(int reg = 0)            { return raw_params[reg].gain1.full; };
	int GetGain2(int reg = 0)            { return raw_params[reg].gain2.full; };
	int GetGain3(int reg = 0)            { return raw_params[reg].gain3.full; };
	int GetSat0(int reg = 0)             { return raw_params[reg].sat0; };
	int GetSat1(int reg = 0)             { return raw_params[reg].sat1; };
	int GetSat2(int reg = 0)             { return raw_params[reg].sat2; };
	int GetSat3(int reg = 0)             { return raw_params[reg].sat3; };

	int GetNumberOfPatchesH(int reg = 0) { return raw_params[reg].h_patches; };
	int GetNumberOfPatchesV(int reg = 0) { return raw_params[reg].v_patches; };
	int GetPatchWidth(int reg = 0)       { return raw_params[reg].patch_width; };
	int GetPatchHeight(int reg = 0)      { return raw_params[reg].patch_height; };
	int GetStartX(int reg = 0)           { return raw_params[reg].x_start; };
	int GetStartY(int reg = 0)           { return raw_params[reg].y_start; };
	int GetSkipX(int reg = 0)            { return raw_params[reg].x_skip; };
	int GetSkipY(int reg = 0)            { return raw_params[reg].y_skip; };

	int GetActivePlanes(int reg = 0)     { return raw_params[reg].active_planes; };
	int GetActivePlanesNo(int reg = 0)   { return raw_params[reg].active_planes_no; };

	void buildHistKernel(uint16_t **, uint16_t *, int);
	void HistPadAtLeft(uint16_t **, uint16_t *, int, int);
	void HistPadAtRight(uint16_t **, uint16_t *, int, int);
	void HistMiddleExecutionH(uint16_t **, uint16_t *, int);
	void setHistOutputLine(void);
	void setStatsOutputLine(void);

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class function
	void TryRun(void);

private:
	RawParameters raw_params[2];
	int kernel_size;
	int stats_width;
	int raw_format;
	int bayer_pattern;
	int grgb_imb_en;
	int hot_pix_en;
	int hot_green_pix_en;
	int stats_patch_en;
	int stats_hist_en;
	int data_width;
	int bad_pixel_thr;
	int bad_pixel_used_bits;
	int bad_pixel_mask;
	int bad_pixel_shift_amount;
	int promotion_used_bits;
	int promotion_shift_amount;

	uint32_t plato_dark;
	uint32_t plato_bright;
	uint32_t slope_dark;
	uint32_t slope_bright;

	uint16_t min1, min2, max1, max2;

	uint32_t alpha_rb_cold;
	uint32_t alpha_rb_hot;
	uint32_t alpha_g_cold;
	uint32_t alpha_g_hot;
	uint16_t noise_level;

	FIXED8_8 gain0;
	FIXED8_8 gain1;
	FIXED8_8 gain2;
	FIXED8_8 gain3;

	uint16_t sat0;
	uint16_t sat1;
	uint16_t sat2;
	uint16_t sat3;

	// Histogram statistics
	int hist_kernel_size;
	int hist_bin[BINS_NO];
	int hist_used_bits;
	int hist_mask;
	int hist_shift_amount;
	int hist_max_pad;
	bool hist_active_plane[16];
	bool enable_hist;

	// Accumulation fields
	uint32_t *stats_ptr;
	int h_patches;
	int v_patches;
	int patch_width;
	int patch_height;
	int x_start;
	int y_start;
	int x_skip;
	int y_skip;
	uint32_t patch_accum[MAX_PATCH_NO*MAX_ACTIVE_PLANES_NO*MAX_PATCH_NO];
	bool accum_started  [MAX_PATCH_NO*MAX_ACTIVE_PLANES_NO];
	bool still_to_accum [MAX_PATCH_NO*MAX_ACTIVE_PLANES_NO];
	int row_of_patches;
	int col_of_patches;
	int vp_cnt[MAX_PATCH_NO*MAX_ACTIVE_PLANES_NO];
	int active_planes;
	int active_planes_no;
	int active_planes_cnt;
	bool accum_active_plane[16];
	bool enable_accum;
	int accum_used_bits;
	int accum_mask;
	int accum_shift_amount;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementation of pure virtual base class function
	void *Run(void **, void *);
};

// Comparison function for qsort()
int icompare(const void *, const void *);

#endif // __SIPP_RAW_H__
