#include "imgproc.h"
#include "TestRunner.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

#define PADDING 8

TestRunner DilateTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int DilateCycleCount;

void Dilate_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height, unsigned int k)
{
	unsigned int maxWidth = 1920 + PADDING;
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    DilateTestRunner.Init();
    DilateTestRunner.SetInput("input", src, width + PADDING, maxWidth, k, 0);
    DilateTestRunner.SetInput("kernel", kernel, k, 8, k, 0);
    DilateTestRunner.SetInput("width", width, 0);
    DilateTestRunner.SetInput("height", height, 0);
    DilateTestRunner.SetInput("k", k, 0);

    DilateTestRunner.Run(0);
    DilateCycleCount = DilateTestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(DilateCycleCount - 16)/ (float)width);
    DilateTestRunner.GetOutput("output", 0, width, maxWidth, 1, dst);
}
