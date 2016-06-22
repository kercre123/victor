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
TestFunction* GetInfoTestFunctions(void);
TestFunction* GetRobotTestFunctions(void);
TestFunction* GetPlaypenTestFunctions(void);

bool MotorDetect(void);
TestFunction* GetMotor1TestFunctions(void);
TestFunction* GetMotor2ATestFunctions(void);
TestFunction* GetMotor2BTestFunctions(void);

bool ExtrasDetect(void);
TestFunction* GetExtrasTestFunctions(void);

#endif
