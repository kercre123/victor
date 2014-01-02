#ifndef _CONVOLUTION11x11_ASM_TEST_H_
#define _CONVOLUTION11x11_ASM_TEST_H_

#include "half.h"

extern unsigned int convolution11x11CycleCount;
void Convolution11x11_asm_test(unsigned char** in, unsigned char** out, half conv[121], unsigned long width);

#endif //_CONVOLUTION11x11_ASM_TEST_H_
