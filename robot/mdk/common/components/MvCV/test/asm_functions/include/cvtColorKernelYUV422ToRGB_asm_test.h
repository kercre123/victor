#ifndef _cvtColorKernelYUV422ToRGB_ASM_TEST_H_
#define _cvtColorKernelYUV422ToRGB_TEST_H_

extern unsigned int cvtColorKernelYUV422ToRGBCycleCount;
void cvtColorKernelYUV422ToRGB_asm_test(unsigned char **input, unsigned char **rOut, unsigned char **gOut, unsigned char **bOut, unsigned int width);

#endif //_cvtColorKernelYUV422ToRGB_ASM_TEST_H_
