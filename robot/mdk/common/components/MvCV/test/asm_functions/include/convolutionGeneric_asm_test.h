#ifndef _CONVOLUTION_ASM_TEST_H_
#define _CONVOLUTION_ASM_TEST_H_

#include "half.h"

extern unsigned int convolutionCycleCount;
void Convolution_asm_test(unsigned char** in, unsigned char** out, unsigned long kernelSize, half* conv, unsigned long width);

#endif //_CONVOLUTION_ASM_TEST_H_
