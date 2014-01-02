#ifndef _cvtColorKernelRGBToYUV422_ASM_TEST_H_
#define _cvtColorKernelRGBToYUV422_TEST_H_

extern unsigned int cvtColorKernelRGBToYUV422CycleCount;
void cvtColorKernelRGBToYUV422_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **output, unsigned int width);

#endif //_cvtColorKernelRGBToYUV422_ASM_TEST_H_
