// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Edge operator filter
//
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_EDGEOP_H__
#define __SIPP_EDGEOP_H__

#include "sippBase.h"
#include "fp16.h"

#include <iostream>
#include <string>

#define COEFF_MASK                   0xffffffe0
#define STEP_SQR                     0.0625f
#define STEP_ATAN                    2.5f
#define EIGHTBITS                    0xff
#define SIXTEENBITS                  0xffff

const float sqr_approx_LUT[] =  {1.0f        , 1.001951221f, 1.007782219f, 1.017426287f, 1.030776406f, 1.047690913f,
	                             1.068000468f, 1.091515575f, 1.118033989f, 1.147347484f, 1.179247642f, 1.21353049f ,
						         1.25f       , 1.288470508f, 1.328768227f, 1.370732012f, 1.414213562f
};

const float atan_approx_LUT[] = {0.0f     , 0.043661f, 0.087489f, 0.131652f, 0.176327f, 0.221695f,
	                             0.267949f, 0.315299f, 0.36397f , 0.414214f, 0.466308f, 0.520567f,
							     0.57735f , 0.63707f , 0.700208f, 0.767327f, 0.8391f  , 0.916331f, 1.0f
};

typedef struct {
	int input_mode;
	int output_mode;
	int theta_mode;
	fp16 magn_scale_factor;
	int h_coeffs[6];
	int v_coeffs[6];
} EdgeOperatorParameters;

class EdgeOperatorFilt : public SippBaseFilt {

public:
	EdgeOperatorFilt(SippIrq *pObj = 0,
		int id = 0,
		int k = 0,
		std::string name = "Edge operator filter");
	~EdgeOperatorFilt() {
	}

	void SetInputMode(int, int reg = 0);
	void SetOutputMode(int, int reg = 0);
	void SetThetaMode(int, int reg = 0);
	void SetMagnScf(int, int reg = 0);

	void SetXCoeff_a(int, int reg = 0);
	void SetXCoeff_b(int, int reg = 0);
	void SetXCoeff_c(int, int reg = 0);
	void SetXCoeff_d(int, int reg = 0);
	void SetXCoeff_e(int, int reg = 0);
	void SetXCoeff_f(int, int reg = 0);

	void SetYCoeff_a(int, int reg = 0);
	void SetYCoeff_b(int, int reg = 0);
	void SetYCoeff_c(int, int reg = 0);
	void SetYCoeff_d(int, int reg = 0);
	void SetYCoeff_e(int, int reg = 0);
	void SetYCoeff_f(int, int reg = 0);

	int DetermineAbsoluteAngle(int, fp16, fp16);
	int DetermineSQRLUTPosition(float);
	int DetermineATANLUTPosition(fp16);
	int DetermineCoeffValue(int);

	int GetInputMode(int reg = 0)  { return edge_params[reg].input_mode; };
	int GetOutputMode(int reg = 0) { return edge_params[reg].output_mode; };
	int GetThetaMode(int reg = 0)  { return edge_params[reg].theta_mode; };
	int GetMagnScf(int reg = 0)    { return edge_params[reg].magn_scale_factor.getPackedValue(); };

	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(void);

	// Filter specific implementation of virtual base class TryRun() function
	void TryRun(void);

private:
	EdgeOperatorParameters edge_params[2];
	int kernel_size;
	int input_mode;
	int output_mode;
	int theta_mode;
	fp16 magn_scale_factor;
	int h_coeffs[6];
	int v_coeffs[6];
	fp16 magn_top_clip;	

	// Copies the filter specific programmable parameters from either default or shadow registers
	void SelectParameters();

	// Implementations of pure virtual base class functions
	void *Run(void **, void *);
};

#endif // __SIPP_EDGEOP_H__
