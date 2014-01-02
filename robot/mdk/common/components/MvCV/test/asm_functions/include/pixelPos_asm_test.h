#ifndef _PIXELPOS_ASM_TEST_H_
#define _PIXELPOS_ASM_TEST_H_

extern unsigned int PixelPosCycleCount;

void pixelPos_asm_test(unsigned char** srcAddr, unsigned char* maskAddr, unsigned long width, unsigned char pixelValue, unsigned long* pixelPosition, unsigned char* status);

#endif //_PIXELPOS_ASM_TEST_H_
