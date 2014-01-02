///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     quantize wrapper file
///

#ifndef ASMOPT_QUANTIZEBAND_H_
#define ASMOPT_QUANTIZEBAND_H_


void asmopt_QuantizeBand(const float *xp, int *pi, float istep, int offset, int end);

#endif /* ASMOPT_QUANTIZEBAND_H_ */
