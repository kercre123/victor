#ifndef _CONVOLUTION_SEPARABLE_7X7_ASM_TEST_H_
#define _CONVOLUTION_SEPARABLE_7X7_ASM_TEST_H_

#include "half.h"

extern unsigned int convolutionSeparable7x7CycleCount;
void ConvolutionSeparable7x7_asm_test(unsigned char** in, unsigned char** out, unsigned char** iterm,
		                              half vconv[7], half hconv[7], unsigned long width);

#endif //_CONVOLUTION_SEPARABLE_7X7_ASM_TEST_H_
