#include "LookupTable.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner LookupTable12to8TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int LookupTable12to8CycleCount;

void LUT12to8_asm_test(unsigned short** src, unsigned char** dest, unsigned char* lut, unsigned int width, unsigned int height)
{
    unsigned int maxWidth = 1920;
    
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    LookupTable12to8TestRunner.Init();
    LookupTable12to8TestRunner.SetInput("input", (unsigned char**)src, width * 2, maxWidth, 1, 0);
    LookupTable12to8TestRunner.SetInput("lut", lut, 4096, 0);
    LookupTable12to8TestRunner.SetInput("width", width, 0);
    LookupTable12to8TestRunner.SetInput("height", height, 0);
    LookupTable12to8TestRunner.Run(0);
   
    LookupTable12to8CycleCount = LookupTable12to8TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(LookupTable12to8CycleCount - 11)/ (float)width);
    
    LookupTable12to8TestRunner.GetOutput("output", 0, width, maxWidth, height, dest);
}
