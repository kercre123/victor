#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 4

TestRunner medianFilter5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter5x5CycleCount;

void medianFilter5x5_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
	medianFilter5x5TestRunner.Init();
	medianFilter5x5TestRunner.SetInput("input", inLine, widthLine, maxWidth, 5, 0);
	medianFilter5x5TestRunner.SetInput("widthLine", widthLine, 0);

	medianFilter5x5TestRunner.Run(0);
	medianFilter5x5CycleCount = medianFilter5x5TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(medianFilter5x5CycleCount - 17) / (float)widthLine);
	medianFilter5x5TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
