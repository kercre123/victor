#ifndef _MINMAXKERNEL_ASM_TEST_H_
#define _MINMAXKERNEL_ASM_TEST_H_

extern unsigned int minMaxCycleCount;
void minMaxKernel_asm_test(unsigned char** in, unsigned int width, unsigned int height,
		unsigned char* minVal, unsigned char* maxVal, unsigned char* maskAddr);

#endif //_MINMAXKERNEL_ASM_TEST_H_
