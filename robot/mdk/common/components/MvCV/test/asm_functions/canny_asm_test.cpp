#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerCanny(APP_PATH, APP_MOFPATH, DBG_INTERFACE);

unsigned int CannyCycleCount;

void canny_asm_test(unsigned char** src, unsigned char** dst,
		float threshold1, float threshold2, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int padding = 4;
	unsigned int maxWidth = 1920 + padding;

	testRunnerCanny.Init();
	testRunnerCanny.SetInput("input", src, width + padding, maxWidth, 9, 0);
	testRunnerCanny.SetInput("threshold1", threshold1, 0);
	testRunnerCanny.SetInput("threshold2", threshold2, 0);
	testRunnerCanny.SetInput("width", width, 0);

	testRunnerCanny.Run(0);

	CannyCycleCount = testRunnerCanny.GetVariableValue("cycleCount");

	functionInfo.AddCyclePerPixelInfo((float)(CannyCycleCount - 12)/ (float)width);

	testRunnerCanny.GetOutput("output", 0, width, maxWidth, 1, dst);
}
