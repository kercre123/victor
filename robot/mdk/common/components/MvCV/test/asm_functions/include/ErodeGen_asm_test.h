#ifndef _ERODE_ASM_TEST_H_
#define _ERODE_ASM_TEST_H_

extern unsigned int ErodeCycleCount;

void Erode_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height, unsigned int k);
#endif //_ERODE_ASM_TEST_H_
