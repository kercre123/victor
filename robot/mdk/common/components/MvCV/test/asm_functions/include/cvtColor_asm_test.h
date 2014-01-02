#ifndef _CVTCOLOR_ASM_TEST_H_
#define _CVTCOLOR_ASM_TEST_H_

extern unsigned int cvtColorCycleCount;

void YUVtoRGB_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width);
void RGBtoYUV_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width);

#endif //_ABSDIFF_ASM_TEST_H_
