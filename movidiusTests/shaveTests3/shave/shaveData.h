#ifndef _SHAVE_DATA_H_
#define _SHAVE_DATA_H_

#include "mv_types.h"

#ifndef f32
#define f32 float
#endif

#ifndef f64
#define f64 double
#endif

#define IMAGE_HEIGHT 64
#define IMAGE_WIDTH 64

#define NUM_COORDINATES 12

__attribute__ ((aligned (16))) extern u8 paddedImageRow[IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern s32 outputLine[IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern s32 integralImage[IMAGE_HEIGHT*IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern const s32 expectedFilterOutput[IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern const s32 expectedIntegralImage[IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern u8 interpolationReferenceImage[IMAGE_HEIGHT*IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern f32 pXCoordinates[NUM_COORDINATES];
__attribute__ ((aligned (16))) extern f32 pYCoordinates[NUM_COORDINATES];
__attribute__ ((aligned (16))) extern f32 pOut[NUM_COORDINATES];

#endif // #ifndef _SHAVE_DATA_H_