#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerIntegral(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int integralImageSumCycleCount;

void integralImageSum_asm_test(unsigned char** in, unsigned char** out, long unsigned int* sum, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerIntegral.Init();
	testRunnerIntegral.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerIntegral.SetInput("thisSum", (unsigned char*)sum, width * 4, 0);
	testRunnerIntegral.SetInput("width", width, 0);
	testRunnerIntegral.Run(0);
	integralImageSumCycleCount = testRunnerIntegral.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(integralImageSumCycleCount - 12)/ (float)width);

	testRunnerIntegral.GetOutput("output", 0, width * 4, maxWidth, 1, out);
	testRunnerIntegral.GetOutput("thisSum", 0, width * 4, maxWidth, 1, (unsigned char**)&sum);
}

