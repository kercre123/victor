#ifndef _CONVOLUTION7X1_ASM_TEST_H_
#define _CONVOLUTION7X1_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution7x1CycleCount;
void Convolution7x1_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION7X1_ASM_TEST_H_
