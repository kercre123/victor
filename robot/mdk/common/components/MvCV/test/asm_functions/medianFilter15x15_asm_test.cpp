#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 16

TestRunner medianFilter15x15TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter15x15CycleCount;

void medianFilter15x15_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    medianFilter15x15TestRunner.Init();
    medianFilter15x15TestRunner.SetInput("input", inLine, widthLine + PADDING, maxWidth, 15, 0);
    medianFilter15x15TestRunner.SetInput("widthLine", widthLine, 0);
	medianFilter15x15TestRunner.SetInput("widthLine", widthLine, 0);
	//medianFilter15x15TestRunner.SetOutput("output",0);
	
    medianFilter15x15TestRunner.Run(0);
    medianFilter15x15CycleCount = medianFilter15x15TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(medianFilter15x15CycleCount - 17) / (float)widthLine);
    medianFilter15x15TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
