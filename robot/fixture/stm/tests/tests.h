#ifndef TESTS_H
#define TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void (*TestFunction)(void);

//Debug
void          TestDebugSetDetect(int ms=1000); //Detect()==true for 'ms'
bool          TestDebugDetect(void);
void          TestDebugCleanup(void);
TestFunction* TestDebugGetTests(void);

//Body
bool          TestBodyDetect(void);
void          TestBodyCleanup(void);
TestFunction* TestBody0GetTests(void);
TestFunction* TestBody1GetTests(void);
TestFunction* TestBody2GetTests(void);
TestFunction* TestBody3GetTests(void);

//Head
uint32_t      TestHeadGetPrevESN(void);
bool          TestHeadDetect(void);
void          TestHeadCleanup(void);
TestFunction* TestHead1GetTests(void);
TestFunction* TestHead2GetTests(void);
TestFunction* TestHelper1GetTests(void);

//Backpack
bool          TestBackpackDetect(void);
void          TestBackpackCleanup(void);
TestFunction* TestBackpack1GetTests(void);

//Cube
bool          TestCubeDetect(void);
void          TestCubeCleanup(void);
TestFunction* TestCube0GetTests(void);
TestFunction* TestCube1GetTests(void);
TestFunction* TestCube2GetTests(void);
bool          TestCubeFinishDetect(void);
TestFunction* TestCubeFinish1GetTests(void);
TestFunction* TestCubeFinish2GetTests(void);
TestFunction* TestCubeFinish3GetTests(void);
TestFunction* TestCubeFinishXGetTests(void);
uint32_t cubebootSignature(bool dbg_print=0, int *out_cubeboot_size=0);

//Robot
bool          TestRobotDetect(void);
void          TestRobotCleanup(void);
TestFunction* TestRobot0GetTests(void);
TestFunction* TestRobot1GetTests(void);
TestFunction* TestRobot2GetTests(void);
TestFunction* TestRobot3GetTests(void);
TestFunction* TestRobotInfoGetTests(void);
TestFunction* TestRobotPlaypenGetTests(void);
TestFunction* TestRobotPackoutGetTests(void);
TestFunction* TestRobotLifetestGetTests(void);
TestFunction* TestRobotRecharge0GetTests(void);
TestFunction* TestRobotRecharge1GetTests(void);
TestFunction* TestRobotSoundGetTests(void);
TestFunction* TestRobotFacRevertGetTests(void);
TestFunction* TestRobotEMGetTests(void);

//Motor
bool          TestMotorDetect(void);
void          TestMotorCleanup(void);
TestFunction* TestMotor1LGetTests(void);
TestFunction* TestMotor1HGetTests(void);
TestFunction* TestMotor2LGetTests(void);
TestFunction* TestMotor2HGetTests(void);
TestFunction* TestMotor3LGetTests(void);
TestFunction* TestMotor3HGetTests(void);


#ifdef __cplusplus
}
#endif

#endif //TESTS_H
