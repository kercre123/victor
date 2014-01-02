#ifndef _ERODE5X5_ASM_TEST_H_
#define _ERODE5X5_ASM_TEST_H_

extern unsigned int Erode5x5CycleCount;

void Erode5x5_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height);
#endif //_ERODE5X5_ASM_TEST_H_
