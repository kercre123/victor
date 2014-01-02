#ifndef _ERODE3X3_ASM_TEST_H_
#define _ERODE3X3_ASM_TEST_H_

extern unsigned int Erode3x3CycleCount;

void Erode3x3_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height);
#endif //_ERODE3X3_ASM_TEST_H_
