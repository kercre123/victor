#ifndef CUBE_TEST_H
#define CUBE_TEST_H

#include "hal/portable.h"

// Return true if device is detected on contacts
bool CubeDetect(void);

TestFunction* GetCubeTestFunctions(void);

#endif
