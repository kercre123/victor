///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     This is the implementation of YuvSubsample library.
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <DrvGpioDefines.h>
#include <DrvCpr.h>
#include <DrvGpio.h>
#include "DrvIcbDefines.h"
#include "YUVSubsampleApi.h"
#include <swcShaveLoader.h>
#include <DrvSvu.h>


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
extern void* YUVSUBSAMPLE_sym(YUVSUBSAMPLE_SHAVE,yuv444_to_yuv420);


// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
void YUVSubsample(unsigned char *in, unsigned char *out, unsigned int width, unsigned int height)
{
	//reset shave before starting it
	swcResetShave(YUVSUBSAMPLE_SHAVE);
	
	//Set the stack pointer, just in case we need extra space in there
	DrvSvutIrfWrite(YUVSUBSAMPLE_SHAVE, 19, YUVSUBSAMPLE_STACK);

    //Set the input address pointer to register i18 (according to calling convention,
	DrvSvutIrfWrite(YUVSUBSAMPLE_SHAVE, 18, (unsigned int)in);
	DrvSvutIrfWrite(YUVSUBSAMPLE_SHAVE, 17, (unsigned int)out);
	DrvSvutIrfWrite(YUVSUBSAMPLE_SHAVE, 16, width);
	DrvSvutIrfWrite(YUVSUBSAMPLE_SHAVE, 15, height);

	//start shave
	DrvSvuStart(YUVSUBSAMPLE_SHAVE, (unsigned int)(&YUVSUBSAMPLE_sym(YUVSUBSAMPLE_SHAVE,yuv444_to_yuv420)));
	//wait for shave for finishing the task	
	swcWaitShave(YUVSUBSAMPLE_SHAVE);
	return;
}

