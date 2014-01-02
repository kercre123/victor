#ifndef __MCLDMATRANSFERTYPES_H___
#define __MCLDMATRANSFERTYPES_H___

#include <swcDmaTypes.h>

#define MYRIAD1_MAX_NUMBER_OF_TASKS 4

typedef struct
{
	u8 svu_nr;
	swcDmaTask_t lastTask;
}dmaHandle;

typedef struct 
{
	u32* src;
	u32* dst;
	u32 len;
	u32 srcLine;
	u32 dstLine;
	u32 srcStride;
	u32 dstStride;
	u8 completed;
}dmaTask;

#endif
