#include "imgproc.h"
#include "TestRunner.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

#define PADDING 8

TestRunner Dilate5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Dilate5x5CycleCount;

void Dilate5x5_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height)
{
	unsigned int maxWidth = 1920 + PADDING;
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    Dilate5x5TestRunner.Init();
    Dilate5x5TestRunner.SetInput("input", src, width + PADDING, maxWidth, 5, 0);
    Dilate5x5TestRunner.SetInput("kernel", kernel, 5, 8, 5, 0);
    Dilate5x5TestRunner.SetInput("width", width, 0);
    Dilate5x5TestRunner.SetInput("height", height, 0);

    Dilate5x5TestRunner.Run(0);
    Dilate5x5CycleCount = Dilate5x5TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(Dilate5x5CycleCount - 16)/ (float)width);
    Dilate5x5TestRunner.GetOutput("output", 0, width, maxWidth, 1, dst);
}
