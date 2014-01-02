#ifndef _cvtColorRGBtoNV21_ASM_TEST_H_
#define _cvtColorRGBtoNV21_TEST_H_

extern unsigned int cvtColorRGBtoNV21CycleCount;
void cvtColorRGBtoNV21_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **Yout, unsigned char **UVout,  unsigned int width, unsigned int line);

#endif //_ccvtColorRGBtoNV21_ASM_TEST_H_
