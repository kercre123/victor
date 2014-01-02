// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model
// -----------------------------------------------------------------------------

#ifndef __SIPP_HW_H__
#define __SIPP_HW_H__

#include "registersMyriad2.h"
#include "sippCommon.h"
#include "sippIrq.h"
#include "sippMemCpy.h"
#include "sippMedian.h"
#include "sippSharpen.h"
#include "sippPolyphaseFIR.h"
#include "sippChromaDenoise.h"
#include "sippConvolution.h"
#include "sippEdgeOperator.h"
#include "sippLookUpTable.h"
#include "sippHarrisCorners.h"
#include "sippDebayer.h"
#include "sippRAW.h"
#include "sippLumaDenoise.h"
#include "sippLSC.h"
#include "sippColorCombination.h"
#include "sippDebayerPPM.h"

#include <iostream>

class SippHW {

public:
	SippHW (void (*pIrqCb)(void *) = 0);
	~SippHW () {
	}

	void SetVerbose (int);

	// APB interface methods
	void ApbWrite (int, int, void * = 0);
	int ApbRead (int);

	// NOTE order of object declaration is important here since the constructor
	// passes a reference to irq to each filter object
	SippIrq irq;

	// NOTE MemCpyFilt class is serving as place-holder for filters prior to
	// implementation of specific classes for each filter
	RawFilt raw;               // RAW filter
	LscFilt lsc;               // Lens shading correction filter
	DebayerFilt dbyr;          // De-Bayering filter
	DebayerPPMFilt dbyrppm;    // Debayer post-processing median filter
	ChromaDenoiseFilt cdn;     // Chroma denoise filter
	LumaDenoiseFilt luma;      // Luma denoise filter
	SharpenFilt usm;           // Sharpening filter
	PolyphaseFIRFilt upfirdn;  // Poly-phase FIR filter
	MedianFilt med;            // Median filter
	LookUpTable lut;           // Look-up table
	EdgeOperatorFilt edge;     // Edge operator filter
	ConvolutionFilt conv;      // Programmable convolution filter
	HarrisCornersFilt harr;    // Harris corners filter
	ColorCombinationFilt cc;   // Color combination filter
private:
	// Return void * pointer to this for passing to Irq constructor, this is to
	// facilitate a callback to a member function and elliminates warning about
	// 'this' being used in base member initializer list
	void *myself(void) {
		return (void *)this;
	}
};

extern SippHW sippHwAcc;

#endif // _SIPP_HW_H__
