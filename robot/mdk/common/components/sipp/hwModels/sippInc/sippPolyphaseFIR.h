// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Polyphase FIR filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_UPFIRDN_H__
#define __SIPP_UPFIRDN_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#ifndef KERNEL_CENTRE
#define KERNEL_CENTRE          (kernel_size >> 1) * (kernel_size) + (kernel_size >> 1)
#endif

#define PHASE_NO               16
#define COEFFS_PER_PHASE       7

#define DEBUG_MESSAGES          0

const char upfirdn_kernel_sizes[] = "357";

typedef struct {
	int kernel_size;
	int output_clamp;
	int v_numerator;
	int v_denominator;
	int h_numerator;
	int h_denominator;
	fp16 v_coeffs[PHASE_NO*COEFFS_PER_PHASE];
	fp16 h_coeffs[PHASE_NO*COEFFS_PER_PHASE];
} PolyphaseFIRParameters;

class PolyphaseFIRFilt : public SippBaseFilt {

public:
	PolyphaseFIRFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 7,
		std::string name = "PolyphaseFIR filter");
	~PolyphaseFIRFilt() {
	}

	void SetKernelSize(int, int reg = 0);
	void SetOutputClamp(int, int reg = 0);
	void SetVerticalNumerator(int, int reg = 0);
	void SetVerticalDenominator(int, int reg = 0);
	void SetHorizontalNumerator(int, int reg = 0);
	void SetHorizontalDenominator(int, int reg = 0);
	void SetVerticalPhase(int);
	void SetVerticalDecimationCounter(int);
	void SetVCoeff(int, int, int, int reg = 0);
	void SetHCoeff(int, int, int, int reg = 0);

	int  StillToReceive(void);
	void DetermineVerticalOperation(void);

	int GetKernelSize(int reg = 0)            { return kernel_size; };
	int GetOutputClamp(int reg = 0)           { return output_clamp; };
	int GetVerticalNumerator(int reg = 0)     { return v_numerator; };
	int GetVerticalDenominator(int reg = 0)   { return v_denominator; };
	int GetHorizontalNumerator(int reg = 0)   { return h_numerator; };
	int GetHorizontalDenominator(int reg = 0) { return h_denominator; };
	int GetVerticalPhase()                    { return v_phase; };
	int GetVerticalDecimationCounter()        { return d_count; };

	void UpdateBuffers(void);
	void UpdateOutputBufferNoWrap(void);

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class function
	void TryRun(void);

private:
	PolyphaseFIRParameters upfirdn_params[2];
	int kernel_size;
	int output_clamp;
	int v_numerator;
	int v_denominator;
	int h_numerator;
	int h_denominator;
	int v_phase;
	int d_count;
	fp16 v_coeffs[PHASE_NO*COEFFS_PER_PHASE];    // 16 phases of 7 vertical   coefficients each
	fp16 h_coeffs[PHASE_NO*COEFFS_PER_PHASE];    // 16 phases of 7 horizontal coefficients each
	int coeff_offset;
	int which_line;
	int v_ph[PHASE_NO];
	bool eoof;
	bool out_ln_generated;
	bool  in_ln_consumed;
	bool upscale;
	bool downscale;
	bool valid;

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Specific Run function for the PolyphaseFIR filter under synchronous control
	void RunSync(void **, void *);

	// Implementation of pure virtual base class function
	void *Run(void **, void *);
};

#endif // __SIPP_UPFIRDN_H__
