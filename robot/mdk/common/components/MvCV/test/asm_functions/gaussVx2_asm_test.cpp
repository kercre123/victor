#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerGaussVx2(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int GaussVx2CycleCount;

void GaussVx2_asm_test(unsigned char** input, unsigned char* output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;

	testRunnerGaussVx2.Init();
	testRunnerGaussVx2.SetInput("input", input, width, maxWidth, 5, 0);
	testRunnerGaussVx2.SetInput("width", width, 0);

	testRunnerGaussVx2.Run(0);

	GaussVx2CycleCount = testRunnerGaussVx2.GetVariableValue("cycleCount");

	functionInfo.AddCyclePerPixelInfo((float)(GaussVx2CycleCount - 12)/ (float)width);

	testRunnerGaussVx2.GetOutput("output", 0, width, output);
}
