#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

#define PADDING 4

TestRunner Dilate3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int Dilate3x3CycleCount;

void Dilate3x3_asm_test(unsigned char** src, unsigned char** dst, unsigned char** kernel, unsigned int width, unsigned int height)
{
    unsigned int maxWidth = 1920;
    unsigned int kernelSize = 3;

    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    Dilate3x3TestRunner.Init();
    Dilate3x3TestRunner.SetInput("input", src, width + PADDING, maxWidth, kernelSize, 0);
    Dilate3x3TestRunner.SetInput("matrix", kernel, kernelSize, 4, kernelSize, 0);
    Dilate3x3TestRunner.SetInput("width", width, 0);
    Dilate3x3TestRunner.SetInput("height", height, 0);

    Dilate3x3TestRunner.Run(0);

    Dilate3x3CycleCount = Dilate3x3TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(Dilate3x3CycleCount - 16)/ (float)width);
    
    Dilate3x3TestRunner.GetOutput("output", 0, width, maxWidth, height, dst);
}
