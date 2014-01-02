#ifndef _cvtColorNV21oRGB_ASM_TEST_H_
#define _cvtColorNV21oRGB_TEST_H_

extern unsigned int cvtColorNV21toRGBCycleCount;
void cvtColorNV21toRGB_asm_test(unsigned char **inputY,unsigned char **inputUV, unsigned char **rOut, unsigned char **gOut, unsigned char **bOut, unsigned int width);

#endif //_cvtColorNV21oRGB_ASM_TEST_H_
