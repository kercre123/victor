#ifndef _BOXFILTER3x3_ASM_TEST_H_
#define _BOXFILTER3x3_ASM_TEST_H_

extern unsigned int boxFilter3x3CycleCount;
void boxfilter3x3_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width);

#endif //_BOXFILTER3x3_ASM_TEST_H_
