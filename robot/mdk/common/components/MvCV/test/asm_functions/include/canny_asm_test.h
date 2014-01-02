#ifndef _CANNY_ASM_TEST_H_
#define _CANNY_ASM_TEST_H_

extern unsigned int CannyCycleCount;
void canny_asm_test(unsigned char** src, unsigned char** dst,
		float threshold1, float threshold2, unsigned int width);

#endif //_CANNY_ASM_TEST_H_
