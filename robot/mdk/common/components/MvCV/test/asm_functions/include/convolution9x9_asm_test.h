#ifndef _CONVOLUTION9x9_ASM_TEST_H_
#define _CONVOLUTION9x9_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution9x9CycleCount;
void Convolution9x9_asm_test(unsigned char** in, unsigned char** out, half conv[81], unsigned long width);

#endif //_CONVOLUTION9x9_ASM_TEST_H_
