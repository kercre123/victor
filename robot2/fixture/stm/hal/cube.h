#ifndef __CUBE_H
#define __CUBE_H
#include "portable.h"

typedef struct {
  u32 serial;
  u8  type;
} cubeid_t;

void InitCube(void);
void ProgramCubeTest(u8* rom, int length);
cubeid_t ProgramCubeWithSerial(bool read_serial_only = false); //@return serial#/type id information

#endif
