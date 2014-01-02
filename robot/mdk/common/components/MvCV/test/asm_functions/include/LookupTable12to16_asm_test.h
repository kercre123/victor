#ifndef _LOOKUPTABLE_ASM_TEST_H_
#define _LOOKUPTABLE_ASM_TEST_H_

extern unsigned int LookupTable12to16CycleCount;

void LUT12to16_asm_test(unsigned short** src, unsigned short** dest, unsigned short* lut, unsigned int width, unsigned int height);

#endif //_LOOKUPTABLE_ASM_TEST_H_
