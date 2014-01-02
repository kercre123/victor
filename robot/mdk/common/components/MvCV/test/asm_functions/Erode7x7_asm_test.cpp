#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 8


TestRunner Erode7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Erode7x7CycleCount;

void Erode7x7_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int widthLine, unsigned int height)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    Erode7x7TestRunner.Init();
    Erode7x7TestRunner.SetInput("input", src, widthLine + PADDING, maxWidth, 7, 0);
    Erode7x7TestRunner.SetInput("width", widthLine, 0);
    Erode7x7TestRunner.SetInput("kernel", kernel, 7, 8, 7, 0);

    Erode7x7TestRunner.Run(0);
    Erode7x7CycleCount = Erode7x7TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(Erode7x7CycleCount - 17) / (float)widthLine);
    Erode7x7TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, dst);
}
