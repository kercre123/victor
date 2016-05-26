#ifndef TESTS_H
#define TESTS_H

#include "hal/portable.h"

bool BodyDetect(void);
TestFunction* GetBodyTestFunctions(void);

bool CubeDetect(void);
TestFunction* GetCubeTestFunctions(void);

bool HeadDetect(void);
TestFunction* GetHeadTestFunctions(void);

bool RobotDetect(void);
TestFunction* GetRobotTestFunctions(void);

bool MotorDetect(void);
TestFunction* GetMotorTestFunctions(void);

bool ExtrasDetect(void);
TestFunction* GetExtrasTestFunctions(void);

#endif
