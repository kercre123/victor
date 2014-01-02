#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 4

TestRunner medianFilter3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter3x3CycleCount;

void medianFilter3x3_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
	medianFilter3x3TestRunner.Init();
	medianFilter3x3TestRunner.SetInput("input", inLine, widthLine, maxWidth, 3, 0);
	medianFilter3x3TestRunner.SetInput("widthLine", widthLine, 0);

	medianFilter3x3TestRunner.Run(0);
	medianFilter3x3CycleCount = medianFilter3x3TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(medianFilter3x3CycleCount - 17) / (float)widthLine);
	medianFilter3x3TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
