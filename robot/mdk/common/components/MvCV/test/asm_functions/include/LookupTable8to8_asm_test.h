#ifndef _LOOKUPTABLE_ASM_TEST_H_
#define _LOOKUPTABLE_ASM_TEST_H_

extern unsigned int LookupTableCycleCount;

void LUT8to8_asm_test(unsigned char** src, unsigned char** dest, unsigned char* lut, unsigned int width, unsigned int height);

#endif //_LOOKUPTABLE_ASM_TEST_H_
