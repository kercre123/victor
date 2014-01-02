#ifndef _Laplacian5x5_ASM_TEST_H_
#define _Laplacian5x5_ASM_TEST_H_

#include "half.h"

extern unsigned int Laplacian5x5CycleCount;
void Laplacian5x5_asm_test(unsigned char** in, unsigned char** out, unsigned char width);

#endif //_Laplacian5x5_ASM_TEST_H_
