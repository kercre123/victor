#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 8


TestRunner Erode5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Erode5x5CycleCount;

void Erode5x5_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int widthLine, unsigned int height)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    Erode5x5TestRunner.Init();
    Erode5x5TestRunner.SetInput("input", src, widthLine + PADDING, maxWidth, 5, 0);
    Erode5x5TestRunner.SetInput("width", widthLine, 0);
    Erode5x5TestRunner.SetInput("kernel", kernel, 5, 8, 5, 0);

    Erode5x5TestRunner.Run(0);
    Erode5x5CycleCount = Erode5x5TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(Erode5x5CycleCount - 17) / (float)widthLine);
    Erode5x5TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, dst);
}
