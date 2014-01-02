#include "LookupTable.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner LookupTable10to8TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int LookupTable10to8CycleCount;

void LUT10to8_asm_test(unsigned short** src, unsigned char** dest, unsigned char* lut, unsigned int width, unsigned int height)
{
    unsigned int maxWidth = 1920;
    
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    LookupTable10to8TestRunner.Init();
    LookupTable10to8TestRunner.SetInput("input", (unsigned char**)src, width * 2, maxWidth, 1, 0);
    LookupTable10to8TestRunner.SetInput("lut", lut, 4096, 0);
    LookupTable10to8TestRunner.SetInput("width", width, 0);
    LookupTable10to8TestRunner.SetInput("height", height, 0);
    LookupTable10to8TestRunner.Run(0);
    
    LookupTable10to8CycleCount = LookupTable10to8TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(LookupTable10to8CycleCount - 11)/ (float)width);
    
    LookupTable10to8TestRunner.GetOutput("output", 0, width, maxWidth, height, dest);
}
