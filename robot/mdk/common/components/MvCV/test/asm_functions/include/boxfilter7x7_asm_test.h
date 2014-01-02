#ifndef _BOXFILTER7x7_ASM_TEST_H_
#define _BOXFILTER7x7_ASM_TEST_H_

extern unsigned int boxFilter7x7CycleCount;
void boxfilter7x7_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width);

#endif //_BOXFILTER7x7_ASM_TEST_H_
