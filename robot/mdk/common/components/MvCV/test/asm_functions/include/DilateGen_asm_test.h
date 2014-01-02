#ifndef _DILATE_ASM_TEST_H_
#define _DILATE_ASM_TEST_H_

extern unsigned int DilateCycleCount;

void Dilate_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height, unsigned int k);

#endif //_DILATE_ASM_TEST_H_
