#ifndef _ERODE7X7_ASM_TEST_H_
#define _ERODE7X7_ASM_TEST_H_

extern unsigned int Erode7x7CycleCount;

void Erode7x7_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height);
#endif //_ERODE7X7_ASM_TEST_H_
