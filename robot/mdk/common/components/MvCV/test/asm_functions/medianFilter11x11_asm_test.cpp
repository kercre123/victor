#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 16

TestRunner medianFilter11x11TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int medianFilter11x11CycleCount;

void medianFilter11x11_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    medianFilter11x11TestRunner.Init();
    medianFilter11x11TestRunner.SetInput("input", inLine, widthLine + PADDING, maxWidth, 11, 0);
    medianFilter11x11TestRunner.SetInput("widthLine", widthLine, 0);

    medianFilter11x11TestRunner.Run(0);
    medianFilter11x11CycleCount = medianFilter11x11TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(medianFilter11x11CycleCount - 17) / (float)widthLine);
    medianFilter11x11TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, outLine);
}
