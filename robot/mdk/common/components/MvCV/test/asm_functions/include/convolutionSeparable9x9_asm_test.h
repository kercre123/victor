#ifndef _CONVOLUTION_SEPARABLE_9X9_ASM_TEST_H_
#define _CONVOLUTION_SEPARABLE_9X9_ASM_TEST_H_

#include "half.h"

extern unsigned int convolutionSeparable9x9CycleCount;
void ConvolutionSeparable9x9_asm_test(unsigned char** in, unsigned char** out, unsigned char** iterm,
		                              half vconv[9], half hconv[9], unsigned long width);

#endif //_CONVOLUTION_SEPARABLE_9X9_ASM_TEST_H_
