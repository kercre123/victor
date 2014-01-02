// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator DEBUG functions
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_DEBUG_H__
#define __SIPP_DEBUG_H__

#include "fp16.h"
#include "sippCommon.h"

void showKernel(uint8_t *kernel, int kw, int kh, bool fliph = false, bool flipv = false);
void showKernel(uint16_t *kernel, int kw, int kh, bool fliph = false, bool flipv = false);
void showKernel(int *kernel, int kw, int kh, bool fliph = false, bool flipv = false);
void showKernel(fp16 *kernel, int kw, int kh, bool fliph = false, bool flipv = false);

#endif // __SIPP_DEBUG_H__
