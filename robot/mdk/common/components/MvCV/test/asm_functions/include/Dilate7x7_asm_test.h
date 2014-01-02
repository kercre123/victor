#ifndef _DILATE7X7_ASM_TEST_H_
#define _DILATE7X7_ASM_TEST_H_

extern unsigned int Dilate7x7CycleCount;

void Dilate7x7_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height);

#endif //_DILATE7X7_ASM_TEST_H_
