#include "video.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner AccumulateWeightedTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int AccumulateWeightedCycleCount;

void AccumulateWeighted_asm_test(unsigned char** srcAddr, unsigned char** maskAddr, float** destAddr, unsigned int width, float alpha)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	AccumulateWeightedTestRunner.Init();
	AccumulateWeightedTestRunner.SetInput("input", srcAddr, width, maxWidth, 1, 0);
	AccumulateWeightedTestRunner.SetInput("mask", maskAddr, width, maxWidth, 1, 0);
	AccumulateWeightedTestRunner.SetInput("output", (unsigned char**)destAddr, width * 4, maxWidth * 4, 1, 0);
	AccumulateWeightedTestRunner.SetInput("width", width, 0);
	AccumulateWeightedTestRunner.SetInput("alpha", alpha, 0);

	AccumulateWeightedTestRunner.Run(0);
	AccumulateWeightedCycleCount = AccumulateWeightedTestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(AccumulateWeightedCycleCount - 22) / (float)width);
	AccumulateWeightedTestRunner.GetOutput("output", 0, width, maxWidth, 1, destAddr);
}