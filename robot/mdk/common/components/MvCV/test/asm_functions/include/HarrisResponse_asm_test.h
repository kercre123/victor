#ifndef _HarrisResponse_ASM_TEST_H_
#define _HarrisResponse_TEST_H_

extern unsigned int HarrisResponseCycleCount;

float HarrisResponse_asm_test(unsigned char* data, unsigned int x, unsigned int y, unsigned int step_width, float k);

#endif //_HarrisResponse_ASM_TEST_H_
