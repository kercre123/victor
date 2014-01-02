#ifndef _WARP_AFFINE_TRANSFORM_BILINEAR_ASM_TEST_
#define _WARP_AFFINE_TRANSFORM_BILINEAR_ASM_TEST_

#include "half.h"

extern unsigned int warpAffineBilinearCycleCount;

void WarpAffineTransformBilinear_asm_test(unsigned char** in, unsigned char** out, unsigned int in_width, 
		unsigned int in_height, unsigned int out_width, unsigned int out_height, 
		unsigned int in_stride, unsigned int out_stride, unsigned char* fill_color, half imat[2][3]);

#endif // _WARP_AFFINE_TRANSFORM_BILINEAR_ASM_TEST_
