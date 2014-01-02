#include "imgproc.h"
#include "TestRunner.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

#define PADDING 8

TestRunner Dilate7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Dilate7x7CycleCount;

void Dilate7x7_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height)
{
	unsigned int maxWidth = 1920 + PADDING;
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    Dilate7x7TestRunner.Init();
    Dilate7x7TestRunner.SetInput("input", src, width + PADDING, maxWidth, 7, 0);
    Dilate7x7TestRunner.SetInput("kernel", kernel, 7, 8, 7, 0);
    Dilate7x7TestRunner.SetInput("width", width, 0);
    Dilate7x7TestRunner.SetInput("height", height, 0);

    Dilate7x7TestRunner.Run(0);
    Dilate7x7CycleCount = Dilate7x7TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(Dilate7x7CycleCount - 16)/ (float)width);
    Dilate7x7TestRunner.GetOutput("output", 0, width, maxWidth, 1, dst);
}
