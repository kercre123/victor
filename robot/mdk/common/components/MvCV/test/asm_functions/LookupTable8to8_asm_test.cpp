#include "LookupTable.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner LookupTableTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int LookupTableCycleCount;

void LUT8to8_asm_test(unsigned char** src, unsigned char** dest, unsigned char* lut, unsigned int width, unsigned int height)
{
    unsigned int maxWidth = 1920;
    
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    LookupTableTestRunner.Init();
    LookupTableTestRunner.SetInput("input", src, width, maxWidth, 1, 0);
    LookupTableTestRunner.SetInput("lut", lut, 256, 0);
    LookupTableTestRunner.SetInput("width", width, 0);
    LookupTableTestRunner.SetInput("height", height, 0);
    LookupTableTestRunner.Run(0);

    LookupTableCycleCount = LookupTableTestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(LookupTableCycleCount - 11)/ (float)width);
    
    LookupTableTestRunner.GetOutput("output", 0, width, maxWidth, height, dest);
}
