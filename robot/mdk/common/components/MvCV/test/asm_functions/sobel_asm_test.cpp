#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerSobel(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int SobelCycleCount;

void sobel_asm_test(unsigned char** input, unsigned char** output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int padding = 4;
	unsigned int maxWidth = 1920 + padding;

	testRunnerSobel.Init();
	testRunnerSobel.SetInput("input", input, width + padding, maxWidth, 3, 0);
	testRunnerSobel.SetInput("width", width, 0);

	testRunnerSobel.Run(0);

	SobelCycleCount = testRunnerSobel.GetVariableValue("cycleCount");

	functionInfo.AddCyclePerPixelInfo((float)(SobelCycleCount - 12)/ (float)width);

	testRunnerSobel.GetOutput("output", 0, width, maxWidth, 1, output);
}

