#ifndef TESTS_H
#define TESTS_H

#include "hal/portable.h"

bool BodyDetect(void);
TestFunction* GetBody1TestFunctions(void);
TestFunction* GetBody2TestFunctions(void);
TestFunction* GetBody3TestFunctions(void);

bool CubeDetect(void);
TestFunction* GetCubeTestFunctions(void);

bool HeadDetect(void);
TestFunction* GetHeadTestFunctions(void);

bool RobotDetect(void);
TestFunction* GetInfoTestFunctions(void);
TestFunction* GetRobotTestFunctions(void);
TestFunction* GetPlaypenTestFunctions(void);
TestFunction* GetPackoutTestFunctions(void);
TestFunction* GetLifetestTestFunctions(void);
TestFunction* GetRechargeTestFunctions(void);
TestFunction* GetJamTestFunctions(void);
    
bool MotorDetect(void);
TestFunction* GetMotor1TestFunctions(void);
TestFunction* GetMotor2ATestFunctions(void);
TestFunction* GetMotor2BTestFunctions(void);

bool FinishDetect(void);
TestFunction* GetFinishTestFunctions(void);

#endif
