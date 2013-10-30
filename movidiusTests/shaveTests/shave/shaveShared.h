#ifndef _SHAVE_SHARED_H_
#define _SHAVE_SHARED_H_

#define DATA_LENGTH 20000

__attribute__ ((aligned (8))) extern volatile int input1[DATA_LENGTH];
__attribute__ ((aligned (8))) extern volatile int input2[DATA_LENGTH];

__attribute__ ((aligned (8))) extern volatile int output[DATA_LENGTH];

void simpleLoop();
void simpleVectorLoop();
void intrinsicsLoop();
void assemblyLoop();

#endif // _SHAVE_SHARED_H_