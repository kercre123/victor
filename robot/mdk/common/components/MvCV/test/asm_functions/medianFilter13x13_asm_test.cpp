#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 16

TestRunner medianFilter13x13TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter13x13CycleCount;

void medianFilter13x13_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    medianFilter13x13TestRunner.Init();
    medianFilter13x13TestRunner.SetInput("input", inLine, widthLine + PADDING, maxWidth, 13, 0);
    medianFilter13x13TestRunner.SetInput("widthLine", widthLine, 0);

    medianFilter13x13TestRunner.Run(0);
    medianFilter13x13CycleCount = medianFilter13x13TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(medianFilter13x13CycleCount - 17) / (float)widthLine);
    medianFilter13x13TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
