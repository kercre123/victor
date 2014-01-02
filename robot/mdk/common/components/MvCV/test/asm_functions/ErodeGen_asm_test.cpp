#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

#define PADDING 8


TestRunner ErodeTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int ErodeCycleCount;

void Erode_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height, unsigned int k)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920 + PADDING;
  
    ErodeTestRunner.Init();
    ErodeTestRunner.SetInput("input", src, width + PADDING, maxWidth, k, 0);
    ErodeTestRunner.SetInput("width", width, 0);
    ErodeTestRunner.SetInput("kernel", kernel, k, 8, k, 0);
    ErodeTestRunner.SetInput("k", k, 0);
    ErodeTestRunner.SetInput("height", height, 0);

    ErodeTestRunner.Run(0);
    ErodeCycleCount = ErodeTestRunner.GetVariableValue("cycleCount");
    functionInfo.AddCyclePerPixelInfo((float)(ErodeCycleCount - 17) / (float)width);
    ErodeTestRunner.GetOutput("output", 0, width, maxWidth, 1, dst);
}
