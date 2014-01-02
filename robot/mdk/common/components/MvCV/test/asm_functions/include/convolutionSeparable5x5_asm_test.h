#ifndef _CONVOLUTION_SEPARABLE_5x5_ASM_TEST_H_
#define _CONVOLUTION_SEPARABLE_5x5_ASM_TEST_H_

#include "half.h"

extern unsigned int convolutionSeparable5x5CycleCount;
void ConvolutionSeparable5x5_asm_test(unsigned char** in, unsigned char** out, unsigned char** iterm,
		                              half vconv[5], half hconv[5], unsigned long width);

#endif //_CONVOLUTION_SEPARABLE_5x5_ASM_TEST_H_
