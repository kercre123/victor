#ifndef _CONVOLUTION7x7_ASM_TEST_H_
#define _CONVOLUTION7x7_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution7x7CycleCount;
void Convolution7x7_asm_test(unsigned char** in, unsigned char** out, half conv[49], unsigned long width);

#endif //_CONVOLUTION7x7_ASM_TEST_H_
