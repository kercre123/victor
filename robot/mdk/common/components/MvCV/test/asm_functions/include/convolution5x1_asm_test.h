#ifndef _CONVOLUTION5X1_ASM_TEST_H_
#define _CONVOLUTION5X1_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution5x1CycleCount;
void Convolution5x1_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION5x1_ASM_TEST_H_
