// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Luma denoise filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_LUMA_H__
#define __SIPP_LUMA_H__

#include "sippBase.h"
#include "fp16.h"

#include <cassert>
#include <iostream>
#include <string>

#define O_KERNEL_CENTRE   (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)

#define F2_MASK           0x3

typedef struct {
	int bitpos;
	uint8_t alpha;
	uint8_t lut[32];
	uint32_t f2_lut;
} LumaDenoiseParameters;

class LumaDenoiseFilt : public SippBaseFilt {

public:
	Buffer ref;

	LumaDenoiseFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		int rk = 0,
		std::string name = "Luma denoise filter");
	~LumaDenoiseFilt() {
	}

	void SetBitPosition(int, int reg = 0);
	void SetAlpha(int, int reg = 0);
	void SetLUT0700(int, int reg = 0);
	void SetLUT1508(int, int reg = 0);
	void SetLUT2316(int, int reg = 0);
	void SetLUT3124(int, int reg = 0);
	void SetF2LUT(int, int reg = 0);

	// SAD computation
	void ComputeSAD(uint8_t *, uint16_t *, int, int, int);
	
	int GetBitPosition(int reg = 0) { return luma_params[reg].bitpos; };
	int GetAlpha(int reg = 0)       { return luma_params[reg].alpha; };

	int  IncInputBufferFillLevel (void);
	bool CanRunLuma(int, int);
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
	LumaDenoiseParameters luma_params[2];
	int kernel_size;
	int ref_kernel_size;
	int ref_plane_start_slice;
	int bitpos;
	uint8_t alpha;
	uint8_t lut[32];
	uint32_t f2_lut;
	int ref_min_fill;
	int ref_max_pad;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the Luma denoise filter
	// First parameter  - Array of pointers to original  input lines
	// Third parameter  - Array of pointers to reference input lines
	// Fourth parameter - Pointer to output line
	void Run(void **, void **, void *);

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_LUMA_H__
