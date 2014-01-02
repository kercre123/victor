#ifndef _Laplacian3X3_ASM_TEST_H_
#define _Laplacian3X3_ASM_TEST_H_

#include "half.h"

extern unsigned int Laplacian3x3CycleCount;
void Laplacian3x3_asm_test(unsigned char** in, unsigned char** out, unsigned char width);

#endif //_Laplacian3X3_ASM_TEST_H_
