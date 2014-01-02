#ifndef _medianFilter5x5_ASM_TEST_H_
#define _medianFilter5x5_TEST_H_

extern unsigned int medianFilter5x5CycleCount;
void medianFilter5x5_asm_test(unsigned int widthLine, unsigned char **outLine, unsigned char **inLine);

#endif //_medianFilter5x5_ASM_TEST_H_
