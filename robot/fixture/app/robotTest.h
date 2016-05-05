#ifndef ROBOT_TEST_H
#define ROBOT_TEST_H

#include "hal/portable.h"

// Return true if device is detected on contacts
bool RobotDetect(void);

TestFunction* GetRobotTestFunctions(void);

#endif
