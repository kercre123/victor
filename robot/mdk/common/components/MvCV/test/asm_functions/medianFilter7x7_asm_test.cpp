#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 8

TestRunner medianFilter7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter7x7CycleCount;

void medianFilter7x7_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
	medianFilter7x7TestRunner.Init();
	medianFilter7x7TestRunner.SetInput("input", inLine, widthLine + PADDING, maxWidth, 7, 0);
	medianFilter7x7TestRunner.SetInput("widthLine", widthLine, 0);

	medianFilter7x7TestRunner.Run(0);
	medianFilter7x7CycleCount = medianFilter7x7TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(medianFilter7x7CycleCount - 17) / (float)widthLine);
	medianFilter7x7TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
