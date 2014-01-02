#ifndef _LOOKUPTABLE_ASM_TEST_H_
#define _LOOKUPTABLE_ASM_TEST_H_

extern unsigned int LookupTable10to8CycleCount;

void LUT10to8_asm_test(unsigned short** src, unsigned char** dest, unsigned char* lut, unsigned int width, unsigned int height);

#endif //_LOOKUPTABLE_ASM_TEST_H_
