// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Chroma denoise filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_UVDNS_H__
#define __SIPP_UVDNS_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#define V_KERNEL_CENTRE      (kernel_size >> 1)
#define H_KERNEL_CENTRE(x)   (cdn_horiz_pass_sizes[x] >> 1)
#define ENABLED_PASS(x)      ((hor_enable>>x)&0x1)
#define USE_REF              (cdn_params[reg].ref_enable)
#define USE_REF_HOR          (ref_enable && pass == 0)

typedef struct {
	int hor_enable;
	int ref_enable;
	int limit;
	int force_weights_hor;
	int force_weights_ver;
	int three_plane_mode;
	int hor_ths[3];
	int ver_ths[3];
} ChromaDenoiseParameters;

class ChromaDenoiseFilt : public SippBaseFilt {

public:
	Buffer ref;

	ChromaDenoiseFilt(SippIrq *pObj = 0,
		int id = 0,
		int vk = 0,
		int hk0 = 0,
		int hk1 = 0,
		int hk2 = 0,
		int rk = 0,
		std::string name = "Chroma denoise filter");
	~ChromaDenoiseFilt() {
	}

	void SetHorEnable(int, int reg = 0);
	void SetRefEnable(int, int reg = 0);
	void SetLimit(int, int reg = 0);
	void SetForceWeightsHor(int, int reg = 0);
	void SetForceWeightsVer(int, int reg = 0);
	void SetThreePlaneMode(int, int reg = 0);
	void SetHorThreshold1(int, int reg = 0);
	void SetHorThreshold2(int, int reg = 0);
	void SetHorThreshold3(int, int reg = 0);
	void SetVerThreshold1(int, int reg = 0);
	void SetVerThreshold2(int, int reg = 0);
	void SetVerThreshold3(int, int reg = 0);

	uint8_t ControlDenoising(uint8_t, uint8_t);
	void    AssertThresholds();
	
	int GetHorEnable(int reg = 0)       { return cdn_params[reg].hor_enable; };
	int GetRefEnable(int reg = 0)       { return cdn_params[reg].ref_enable; };
	int GetLimit(int reg = 0)           { return cdn_params[reg].limit; };
	int GetForceWeightsHor(int reg = 0) { return cdn_params[reg].force_weights_hor; };
	int GetForceWeightsVer(int reg = 0) { return cdn_params[reg].force_weights_ver; };
	int GetThreePlaneMode(int reg = 0)  { return cdn_params[reg].three_plane_mode; };
	int GetHorThreshold1(int reg = 0)   { return cdn_params[reg].hor_ths[1]; };
	int GetHorThreshold2(int reg = 0)   { return cdn_params[reg].hor_ths[2]; };
	int GetHorThreshold3(int reg = 0)   { return cdn_params[reg].hor_ths[3]; };
	int GetVerThreshold1(int reg = 0)   { return cdn_params[reg].ver_ths[1]; };
	int GetVerThreshold2(int reg = 0)   { return cdn_params[reg].ver_ths[2]; };
	int GetVerThreshold3(int reg = 0)   { return cdn_params[reg].ver_ths[3]; };

	int  IncInputBufferFillLevel (void);
	bool CanRunCdn(int, int);
	void getReferenceLines (uint8_t **, int);
	void PackReferenceLines(uint8_t **, int, uint8_t **);
	int  NextReferenceSlice(int offset = 0);
	void ComputeReferenceStartSlice(int);
	void PadAtTop(uint8_t **, int, int);
	void PadAtBottom(uint8_t **, int, int);
	void MiddleExecutionV(uint8_t **, int);
	void UpdateBuffers(void);
	void UpdateBuffersSync(void);
	void UpdateReferenceBuffer(void);
	void UpdateReferenceBufferSync(void);
	void UpdateReferenceBufferNoWrap(void);

	void buildRefKernel(uint8_t **, uint8_t *, int);
	void PadAtLeft(uint8_t **, uint8_t *, int, int);
	void PadAtRight(uint8_t **, uint8_t *, int, int);
	void MiddleExecutionH(uint8_t **, uint8_t *, int);

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	ChromaDenoiseParameters cdn_params[2];
	int kernel_size;
	int ref_kernel_size;
	int ref_plane_start_slice;
	int hor_enable;
	int ref_enable;
	int limit;
	int force_weights_hor;
	int force_weights_ver;
	int three_plane_mode;
	int hor_ths[3];
	int ver_ths[3];
	int ref_min_fill;
	int ref_max_pad;
	int cdn_horiz_pass_sizes[3];

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the Chroma denoise filter when reference is used
	// First  parameter  - Array of pointers to original  input lines
	// Second parameter  - Array of pointers to reference input lines
	// Third  parameter  - Pointer to output line
	void Run(void **, void **, void *);

	// Specific Run function for the Chroma denoise filter when reference is not used
	// First  parameter - Array of pointers to original input planes
	// Second parameter - Array of pointers to output lines
	void Run(void ***, void **);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_UVDNS_H__
