#ifndef _EQUALIZEHIST_ASM_TEST_H_
#define _EQUALIZEHIST_ASM_TEST_H_

extern unsigned int equalizeHistCycleCount;
void equalizeHist_asm_test(unsigned char** in, unsigned char** out, long unsigned int* hist, unsigned int width);

#endif //_EQUALIZEHIST_ASM_TEST_H_
