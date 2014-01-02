#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"

TestRunner testRunnerPos(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int minMaxCycleCountPos;

void minMaxPos_asm_test(unsigned char** in, unsigned int width,
		unsigned char* minVal, unsigned char* maxVal, long unsigned int* minPos, long unsigned int* maxPos, unsigned char* maskAddr)
{
	unsigned int maxWidth = 1920;
	FunctionInfo& functionInfo = FunctionInfo::Instance();

	testRunnerPos.Init();
	testRunnerPos.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerPos.SetInput("width", width, 0);
	testRunnerPos.SetInput("minVal", minVal, 1, 0);
	testRunnerPos.SetInput("maxVal", maxVal, 1, 0);
	testRunnerPos.SetInput("minPos", (unsigned int)*minPos, 0);
	testRunnerPos.SetInput("maxPos", (unsigned int)*maxPos, 0);
	testRunnerPos.SetInput("mask", maskAddr, width, 0);

	testRunnerPos.Run(0);

	minMaxCycleCountPos = testRunnerPos.GetVariableValue(std::string("cycleCount"));
	functionInfo.AddCyclePerPixelInfo((float)(minMaxCycleCountPos - 5)/ (float)width);

	testRunnerPos.GetOutput("minVal", 0, minVal);
	testRunnerPos.GetOutput("maxVal", 0, maxVal);
	testRunnerPos.GetOutput("minPos", 0, minPos);
	testRunnerPos.GetOutput("maxPos", 0, maxPos);
}
