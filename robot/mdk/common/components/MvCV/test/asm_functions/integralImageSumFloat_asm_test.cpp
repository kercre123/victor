#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerIntegralF(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int integralImageSumFCycleCount;

void integralImageSumF_asm_test(unsigned char** in, unsigned char** out, long unsigned int* sum, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerIntegralF.Init();
	testRunnerIntegralF.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerIntegralF.SetInput("thisSum", (unsigned char*)sum, width * 4, 0);
	testRunnerIntegralF.SetInput("width", width, 0);
	testRunnerIntegralF.Run(0);
	integralImageSumFCycleCount = testRunnerIntegralF.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(integralImageSumFCycleCount - 12)/ (float)width);

	testRunnerIntegralF.GetOutput("output", 0, width * sizeof(float), maxWidth, 1, out);
	testRunnerIntegralF.GetOutput("thisSum", 0, width * 4, maxWidth, 1, (unsigned char**)&sum);
}

