// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - LSC filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_LSC_H__
#define __SIPP_LSC_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

typedef struct {
	FIXED0_16 inc_h;
	FIXED0_16 inc_v;
	int lsc_format;
	int data_width;
	int lsc_gm_width;
	int lsc_gm_height;
} LscParameters;

class LscFilt : public SippBaseFilt {

public:
	Buffer lsc_gmb;

	LscFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		std::string name = "LSC filter");
	~LscFilt() {
	}

	void Reset(void);

	void SetHorIncrement(int, int reg = 0);
	void SetVerIncrement(int, int reg = 0);
	void SetLscFormat(int, int reg = 0);
	void SetDataWidth(int, int reg = 0);
	void SetLscGmWidth(int, int reg = 0);
	void SetLscGmHeight(int, int reg = 0);

	void VerticalAdvance(void);

	void BayerAdjustHCoordinates(int);
	void PlanarAdjustHCoordinates(int);

	void DetermineUsedMesh(int);

	int GetHorIncrement(int reg = 0)  { return lsc_params[reg].inc_v.full; };
	int GetVerIncrement(int reg = 0)  { return lsc_params[reg].inc_h.full; };
	int GetLscFormat(int reg = 0)     { return lsc_params[reg].lsc_format; };
	int GetDataWidth(int reg = 0)     { return lsc_params[reg].data_width; };
	int GetLscGmWidth(int reg = 0)    { return lsc_params[reg].lsc_gm_width; };
	int GetLscGmHeight(int reg = 0)   { return lsc_params[reg].lsc_gm_height; };

	void getGainMapLines(FIXED8_8 ***);

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class function
	void TryRun(void);

private:
	LscParameters lsc_params[2];
	FIXED0_16 inc_h;
	FIXED0_16 inc_v;
	int kernel_size;
	int lsc_format;
	int data_width;
	int lsc_gm_width;
	int lsc_gm_height;
	int actual_width;
	int actual_height;
	bool update_gm_idx;
	bool no_vert_interp;
	bool no_vert_interp_next;
	bool no_horz_interp;
	bool no_horz_interp_next;
	FIXED16_16 u;
	FIXED16_16 v;
	uint32_t i0;
	uint32_t i1;
	uint32_t j0;
	uint16_t x_even;
	uint16_t y_even;
	int gm_crt_plane;
	int gm_crt_line1;
	int gm_crt_line2;
	int gm_lines;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the LSC filter
	void Run(void **, void***, void *);

	// Implementation of pure virtual base class function
	void *Run(void **, void *);
};

#endif // __SIPP_LSC_H__
