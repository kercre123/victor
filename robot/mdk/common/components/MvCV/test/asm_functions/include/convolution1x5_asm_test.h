#ifndef _CONVOLUTION1x5_ASM_TEST_H_
#define _CONVOLUTION1x5_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution1x5CycleCount;
void Convolution1x5_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION1x5_ASM_TEST_H_
