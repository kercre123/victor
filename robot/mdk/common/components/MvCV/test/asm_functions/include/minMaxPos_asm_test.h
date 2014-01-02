#ifndef _MINMAXPOS_ASM_TEST_H_
#define _MINMAXPOS_ASM_TEST_H_

extern unsigned int minMaxCycleCountPos;
void minMaxPos_asm_test(unsigned char** in, unsigned int width,
 unsigned char* minVal, unsigned char* maxVal, long unsigned int* minPos,long unsigned int* maxPos, unsigned char* maskAddr);

#endif //_MINMAXPOS_ASM_TEST_H_
