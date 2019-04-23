/*
 * file: conv.h   Unit conversion routines.
 * 
 * Description:  Provides blocksize, sample rate, etc conversion routines
 *
 */
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _CONV_H_
#define _CONV_H_


#include "se_types.h"

int     Conv_TimeToSize(float32 rateHz, float32 blockTimeS);
float32 Conv_SizeToTime(float32 rateHz, int blockSize);

int32   Conv_RateTokHz(float32 rateHz);
float32 Conv_kHzToRate(int ratekHz);

#endif
#ifdef __cplusplus
}
#endif
