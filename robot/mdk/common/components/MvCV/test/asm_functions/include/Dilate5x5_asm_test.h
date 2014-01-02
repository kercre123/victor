#ifndef _DILATE5X5_ASM_TEST_H_
#define _DILATE5X5_ASM_TEST_H_

extern unsigned int Dilate5x5CycleCount;

void Dilate5x5_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height);

#endif //_DILATE5X5_ASM_TEST_H_
