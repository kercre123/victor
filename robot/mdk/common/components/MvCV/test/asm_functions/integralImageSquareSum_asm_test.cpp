#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerIntegralSquare(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int integralImageSquareSumCycleCount;

void integralImageSquareSum_asm_test(unsigned char** in, unsigned char** out, long unsigned int* sum, unsigned int width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerIntegralSquare.Init();
	testRunnerIntegralSquare.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerIntegralSquare.SetInput("thisSum", (unsigned char*)sum, width * 4, 0);
	testRunnerIntegralSquare.SetInput("width", width, 0);
	testRunnerIntegralSquare.Run(0);
	integralImageSquareSumCycleCount = testRunnerIntegralSquare.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(integralImageSquareSumCycleCount - 12)/ (float)width);

	testRunnerIntegralSquare.GetOutput("output", 0, width * 4, maxWidth, 1, out);
	testRunnerIntegralSquare.GetOutput("thisSum", 0, width * 4, maxWidth, 1, (unsigned char**)&sum);
}

