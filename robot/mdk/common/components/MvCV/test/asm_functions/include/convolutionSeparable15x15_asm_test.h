#ifndef _CONVOLUTION_SEPARABLE_15X15_ASM_TEST_H_
#define _CONVOLUTION_SEPARABLE_15X15_ASM_TEST_H_

#include "half.h"

extern unsigned int convolutionSeparable15x15CycleCount;
void ConvolutionSeparable15x15_asm_test(unsigned char** in, unsigned char** out, unsigned char** iterm,
		                              half vconv[9], half hconv[9], unsigned long width);

#endif //_CONVOLUTION_SEPARABLE_15X15_ASM_TEST_H_
