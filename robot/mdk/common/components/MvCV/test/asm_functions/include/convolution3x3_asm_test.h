#ifndef _CONVOLUTION3X3_ASM_TEST_H_
#define _CONVOLUTION3X3_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution3x3CycleCount;
void Convolution3x3_asm_test(unsigned char** in, unsigned char** out, half conv[9], unsigned long width);

#endif //_CONVOLUTION3X3_ASM_TEST_H_
