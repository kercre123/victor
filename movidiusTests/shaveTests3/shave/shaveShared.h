#ifndef _SHAVE_SHARED_H_
#define _SHAVE_SHARED_H_

#define IMAGE_HEIGHT 64
#define IMAGE_WIDTH 64

#define NUM_COORDINATES 12

__attribute__ ((aligned (16))) extern volatile int outputLine[IMAGE_WIDTH];
__attribute__ ((aligned (16))) extern volatile unsigned char paddedImageRow[IMAGE_WIDTH];
__attribute__ ((aligned (16))) extern int integralImage[IMAGE_HEIGHT*IMAGE_WIDTH];

__attribute__ ((aligned (16))) extern unsigned char interpolationReferenceImage[IMAGE_HEIGHT*IMAGE_WIDTH];
__attribute__ ((aligned (16))) extern float pXCoordinates[NUM_COORDINATES];
__attribute__ ((aligned (16))) extern float pYCoordinates[NUM_COORDINATES];
__attribute__ ((aligned (16))) extern float pOut[NUM_COORDINATES];

void testIntegralImageFiltering();

void testIntegralImageGeneration();

void testInterp2_reference();
void testInterp2_shave();

#endif // _SHAVE_SHARED_H_