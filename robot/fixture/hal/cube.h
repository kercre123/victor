#ifndef __CUBE_H
#define __CUBE_H
#include "portable.h"

void InitCube(void);
void ProgramCubeTest(u8* rom, int length);
u32  ProgramCubeWithSerial(bool read_serial_only = false); //@return serial#

#endif
