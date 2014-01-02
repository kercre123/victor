#ifndef _CONVOLUTION15X1_ASM_TEST_H_
#define _CONVOLUTION15X1_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution15x1CycleCount;
void Convolution15x1_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width);

#endif //_CONVOLUTION15x1_ASM_TEST_H_
