#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 4

TestRunner sumOfAbsDiff5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int sumOfAbsDiff5x5CycleCount;

void sumOfAbsDiff5x5_asm_test(unsigned char **input1, unsigned char **input2, unsigned char **output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
	sumOfAbsDiff5x5TestRunner.Init();
	sumOfAbsDiff5x5TestRunner.SetInput("input1", input1, width, maxWidth, 5, 0);
	sumOfAbsDiff5x5TestRunner.SetInput("input2", input2, width, maxWidth, 5, 0);
	sumOfAbsDiff5x5TestRunner.SetInput("width", width, 0);

	sumOfAbsDiff5x5TestRunner.Run(0);
	
	sumOfAbsDiff5x5CycleCount = sumOfAbsDiff5x5TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(sumOfAbsDiff5x5CycleCount - 29) / (float)width);
	
	sumOfAbsDiff5x5TestRunner.GetOutput("output", 0, width, maxWidth, 1, output);

}
