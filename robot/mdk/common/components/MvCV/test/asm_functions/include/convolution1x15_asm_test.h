#ifndef _CONVOLUTION1x15_ASM_TEST_H_
#define _CONVOLUTION1x15_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution1x15CycleCount;
void Convolution1x15_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION1x15_ASM_TEST_H_
