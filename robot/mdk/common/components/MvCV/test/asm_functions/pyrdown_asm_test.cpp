#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerPyrDown(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int PyrDownCycleCount;

void pyrdown_asm_test(unsigned char** input, unsigned char** output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int padding = 4;
	unsigned int maxWidth = 1920 + padding;

	testRunnerPyrDown.Init();
	testRunnerPyrDown.SetInput("input", input, width + padding, maxWidth, 5, 0);
	testRunnerPyrDown.SetInput("width", width, 0);

	testRunnerPyrDown.Run(0);

	PyrDownCycleCount = testRunnerPyrDown.GetVariableValue("cycleCount");

	functionInfo.AddCyclePerPixelInfo((float)(PyrDownCycleCount - 12)/ (float)width);

	testRunnerPyrDown.GetOutput("output", 0, (width - 1) / 2 + 1, maxWidth, 1, output);
}
