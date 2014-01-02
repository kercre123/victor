#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerIntegralSquareF(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int integralImageSquareSumFCycleCount;

void integralImageSquareSumFloat_asm_test(unsigned char** in, unsigned char** out, float* sum, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerIntegralSquareF.Init();
	testRunnerIntegralSquareF.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerIntegralSquareF.SetInput("thisSum", (unsigned char*)sum, width * 4, 0);
	testRunnerIntegralSquareF.SetInput("width", width, 0);
	testRunnerIntegralSquareF.Run(0);
	integralImageSquareSumFCycleCount = testRunnerIntegralSquareF.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(integralImageSquareSumFCycleCount - 12)/ (float)width);

	testRunnerIntegralSquareF.GetOutput("output", 0, width * sizeof(float), maxWidth, 1, out);
	testRunnerIntegralSquareF.GetOutput("thisSum", 0, width * 4, maxWidth, 1, (unsigned char**)&sum);
}

