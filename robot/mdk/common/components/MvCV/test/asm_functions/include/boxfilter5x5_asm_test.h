#ifndef _BOXFILTER5x5_ASM_TEST_H_
#define _BOXFILTER5x5_ASM_TEST_H_

extern unsigned int boxFilter5x5CycleCount;
void boxfilter5x5_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width);

#endif //_BOXFILTER5x5_ASM_TEST_H_
