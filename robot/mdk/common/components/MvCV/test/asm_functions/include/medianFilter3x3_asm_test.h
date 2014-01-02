#ifndef _medianFilter3x3_ASM_TEST_H_
#define _medianFilter3x3_TEST_H_

extern unsigned int medianFilter3x3CycleCount;
void medianFilter3x3_asm_test(unsigned int widthLine, unsigned char **outLine, unsigned char **inLine);

#endif //_medianFilter3x3_ASM_TEST_H_
