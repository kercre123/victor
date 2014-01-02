#include "harris_score.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define WIDTH 8
#define HEIGHT 8

TestRunner HarrisResponseTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int HarrisResponseCycleCount;

float HarrisResponse_asm_test(unsigned char* data, unsigned int x, unsigned int y, unsigned int step_width, float k)
{
	float out = 0;
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	HarrisResponseTestRunner.Init();
	HarrisResponseTestRunner.SetInput("input", data, WIDTH * HEIGHT, 0);
	HarrisResponseTestRunner.SetInput("x", x, 0);
	HarrisResponseTestRunner.SetInput("y", y, 0);
	HarrisResponseTestRunner.SetInput("step_width", step_width, 0);
	HarrisResponseTestRunner.SetInput("k", k, 0);

	HarrisResponseTestRunner.Run(0);
	HarrisResponseCycleCount = HarrisResponseTestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(HarrisResponseCycleCount - 3) / (float)WIDTH);
	HarrisResponseTestRunner.GetOutput("output", 0, 1, &out);
	
	return out;
}
