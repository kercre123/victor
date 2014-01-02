#ifndef _CONVOLUTION5x5_ASM_TEST_H_
#define _CONVOLUTION5x5_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution5x5CycleCount;
void Convolution5x5_asm_test(unsigned char** in, unsigned char** out, half conv[25], unsigned long width);

#endif //_CONVOLUTION5x5_ASM_TEST_H_
