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
uint32_t cubebootSignature(bool dbg_print=0, int *out_cubeboot_size=0);
bool          TestCubeDetect(void);
void          TestCubeCleanup(void);
TestFunction* TestCubeOLGetTests(void);
TestFunction* TestCubeFccGetTests(void);
TestFunction* TestCube0GetTests(void);
TestFunction* TestCube1GetTests(void);
TestFunction* TestCube2GetTests(void);
bool          TestBlockDetect(void);
void          TestBlockCleanup(void);
TestFunction* TestBlock1GetTests(void);
TestFunction* TestBlock2GetTests(void);

//Robot
bool          TestRobotDetect(void);
void          TestRobotCleanup(void);
TestFunction* TestRobot0GetTests(void);
TestFunction* TestRobot1GetTests(void);
TestFunction* TestRobot2GetTests(void);
TestFunction* TestRobot3GetTests(void);
TestFunction* TestRobotInfoGetTests(void);
TestFunction* TestRobotPackoutGetTests(void);
TestFunction* TestRobotRechargeGetTests(void);
TestFunction* TestRobotSoundGetTests(void);

//Motor
bool          TestMotorDetect(void);
void          TestMotorCleanup(void);
TestFunction* TestMotor1GetTests(void);
TestFunction* TestMotor2GetTests(void);
TestFunction* TestMotor3GetTests(void);


#ifdef __cplusplus
}
#endif

#endif //TESTS_H
