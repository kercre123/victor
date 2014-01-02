#ifndef _CONVOLUTION9X1_ASM_TEST_H_
#define _CONVOLUTION9X1_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution9x1CycleCount;
void Convolution9x1_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION9X1_ASM_TEST_H_
