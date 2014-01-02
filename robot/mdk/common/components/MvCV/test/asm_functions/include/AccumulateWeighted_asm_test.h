#ifndef _AccumulateWeighted_ASM_TEST_H_
#define _AccumulateWeighted_TEST_H_

extern unsigned int AccumulateWeightedCycleCount;
void AccumulateWeighted_asm_test(unsigned char** srcAddr, unsigned char** maskAddr, float** destAddr, unsigned int width, float alpha);

#endif //_AccumulateWeighted_ASM_TEST_H_
