///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave kernel example
///

#include <mv_types.h>
#include <video.h>
#include <string.h>
#include "stdio.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
namespace mvcv{
#endif

void AccumulateSquare(u8** srcAddr, u8** maskAddr, fp32** destAddr, u32 width, u32 height)
{
	u32 i,j;
	u8* src;
	u8* mask;
	fp32* dest;

	src = *srcAddr;
	mask = *maskAddr;
	dest = *destAddr;

	for (i = 0; i < height; i++){
		for (j = 0;j < width; j++){
			if(mask[j + i * width]){
				dest[j + i * width] =  dest[j + i * width] + src[j + i * width] * src[j + i * width];
			}
		}
	}
	return;
}


void AccumulateWeighted(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, fp32 alpha)
{
	u32 i;
	u8* src;
	u8* mask;
	fp32* dest;
	
	src = *srcAddr;
	mask = *maskAddr;
	dest = *destAddr;
	
	for (i = 0;i < width; i++)
	{
		if(mask[i]){
				dest[i] =  (1 - alpha) * dest[i] + alpha * src[i];
		}		
	}
	
	return;
}

#ifdef __cplusplus
}
#endif

