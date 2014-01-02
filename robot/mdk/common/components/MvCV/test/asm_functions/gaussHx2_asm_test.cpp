#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerGaussHx2(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int GaussHx2CycleCount;

void GaussHx2_asm_test(unsigned char* input, unsigned char* output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int padding = 4;
	testRunnerGaussHx2.Init();
	testRunnerGaussHx2.SetInput("input", input, width + padding, 0);
	testRunnerGaussHx2.SetInput("width", width, 0);

	testRunnerGaussHx2.Run(0);

	GaussHx2CycleCount = testRunnerGaussHx2.GetVariableValue("cycleCount");

	functionInfo.AddCyclePerPixelInfo((float)(GaussHx2CycleCount - 12)/ (float)width);

	testRunnerGaussHx2.GetOutput("output", 0, width / 2, output);
}
