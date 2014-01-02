#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 8

TestRunner medianFilter9x9TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter9x9CycleCount;

void medianFilter9x9_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    medianFilter9x9TestRunner.Init();
    medianFilter9x9TestRunner.SetInput("input", inLine, widthLine + PADDING, maxWidth, 9, 0);
    medianFilter9x9TestRunner.SetInput("widthLine", widthLine, 0);

    medianFilter9x9TestRunner.Run(0);
    medianFilter9x9CycleCount = medianFilter9x9TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(medianFilter9x9CycleCount - 17) / (float)widthLine);
    medianFilter9x9TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
