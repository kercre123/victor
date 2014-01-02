#ifndef _BOXFILTER_ASM_TEST_H_
#define _BOXFILTER_ASM_TEST_H_

extern unsigned int boxFilterCycleCount;
void boxfilter_asm_test(unsigned char** in, unsigned char** out, unsigned long kernelSize, unsigned long normalize, unsigned long width);

#endif //_BOXFILTER_ASM_TEST_H_
