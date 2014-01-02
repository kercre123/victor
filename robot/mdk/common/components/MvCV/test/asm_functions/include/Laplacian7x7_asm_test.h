#ifndef _Laplacian7x7_ASM_TEST_H_
#define _Laplacian7x7_ASM_TEST_H_

#include "half.h"

extern unsigned int Laplacian7x7CycleCount;
void Laplacian7x7_asm_test(unsigned char** in, unsigned char** out, unsigned char width);

#endif //_Laplacian7x7_ASM_TEST_H_
