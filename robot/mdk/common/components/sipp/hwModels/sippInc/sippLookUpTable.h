// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Look up table
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_LUT_H__
#define __SIPP_LUT_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

typedef struct {
	int interp_mode_en;
	int channel_mode_en;
	int int_width;
	int luts_no;
	int channels_no;
	int regions[16];
} LookUpTableParameters;

class LookUpTable : public SippBaseFilt {

public:
	Buffer loader;

	LookUpTable(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		std::string name = "Look up table");
	~LookUpTable() {
	}

	void SetInterpolationModeEnable(int, int reg = 0);
	void SetChannelModeEnable(int, int reg = 0);
	void SetIntegerWidth(int, int reg = 0);
	void SetNumberOfLUTs(int, int reg = 0);
	void SetNumberOfChannels(int, int reg = 0);
	void SetLutLoadEnable(int);
	void SetApbAccessEnable(int);
	void SetRegions7_0(int, int reg = 0);
	void SetRegions15_8(int, int reg = 0);

	// APB debug
	void SetLutValue(int);
	void SetRamAddress(int);
	void SetRamInstance(int);
	void SetLutRW(int);
	int  GetLutValue()               { return  GetInterpolationModeEnable() == DISABLED ? lut_getintval : lut_getfpval.getPackedValue(); };

	int SelectRangeFP16(fp16);
	int SelectRangeINT(int);
	int ComputeRawOffsetFP16(fp16);
	int ComputeRawOffsetINT(int);
	int ComputeAddrOffsetFP16(fp16);
	int ComputeAddrOffsetINT(int);
	int ComputeIN(fp16);
	size_t ComputeLutAddrFP16(fp16);
	size_t ComputeLutAddrINT(int);
	
	int ComputeChannelAddr(int);

	int GetLutSize();
	int GetLufSize();

	int GetInterpolationModeEnable(int reg = 0) { return lut_params[reg].interp_mode_en; };
	int GetChannelModeEnable(int reg = 0)       { return lut_params[reg].channel_mode_en; };
	int GetIntegerWidth(int reg = 0)            { return lut_params[reg].int_width; };
	int GetNumbefOfLUTs(int reg = 0)            { return lut_params[reg].luts_no; };
	int GetNumberOfChannels(int reg = 0)        { return lut_params[reg].channels_no; };
	int GetLutLoadEnable()                      { return lut_load_en; };
	int GetApbAccessEnable()                    { return apb_access_en; };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	LookUpTableParameters lut_params[2];
	int kernel_size;
	int interp_mode_en;
	int channel_mode_en;
	int int_width;
	int luts_no;
	int channels_no;
	int lut_load_en;
	int apb_access_en;
	int regions[16];
	int crt_plane;
	int crt_channel;
	int crt_output_sp;
	int  interp_offset;
	bool perform_interp;
	// APB debug
	uint16_t lut_setintval;
	fp16     lut_setfpval;
	int ram_addr;
	int ram_inst;
	int rw;
	uint16_t lut_getintval;
	fp16     lut_getfpval;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the LUT
	void Run(void ***, void **);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_LUT_H__
