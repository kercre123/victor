#ifndef _CONVOLUTION1x9_ASM_TEST_H_
#define _CONVOLUTION1x9_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution1x9CycleCount;
void Convolution1x9_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION1x9_ASM_TEST_H_
