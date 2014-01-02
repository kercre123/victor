#ifndef _thresholdKernel_ASM_TEST_H_
#define _thresholdKernel_ASM_TEST_H_

extern unsigned int thresholdKernelCycleCount;
void thresholdKernel_asm_test(unsigned char** in, unsigned char** out, unsigned int width, unsigned int height, unsigned char thresh, unsigned int thresh_type);

#endif //_thresholdKernel_ASM_TEST_H_
