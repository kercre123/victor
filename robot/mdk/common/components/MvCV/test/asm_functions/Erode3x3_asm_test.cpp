#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 4


TestRunner Erode3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Erode3x3CycleCount;

void Erode3x3_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int widthLine, unsigned int height)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
    Erode3x3TestRunner.Init();
    Erode3x3TestRunner.SetInput("input", src, widthLine + PADDING, maxWidth, 3, 0);
    Erode3x3TestRunner.SetInput("width", widthLine, 0);
    Erode3x3TestRunner.SetInput("kernel", kernel, 3, 4, 3, 0);

    Erode3x3TestRunner.Run(0);
    Erode3x3CycleCount = Erode3x3TestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(Erode3x3CycleCount - 17) / (float)widthLine);
    Erode3x3TestRunner.GetOutput("output", 0, widthLine, maxWidth, 1, dst);
}
