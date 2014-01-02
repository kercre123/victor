#include "LookupTable.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner LookupTable12to16TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int LookupTable12to16CycleCount;

void LUT12to16_asm_test(unsigned short** src, unsigned short** dest, unsigned short* lut, unsigned int width, unsigned int height)
{
    unsigned int maxWidth = 1920;
    
    FunctionInfo& functionInfo = FunctionInfo::Instance();
    
    LookupTable12to16TestRunner.Init();
    LookupTable12to16TestRunner.SetInput("input", (unsigned char**)src, width * 2, maxWidth, 1, 0);
    LookupTable12to16TestRunner.SetInput("lut", (unsigned char*)lut, 4096 * 2, 0);
    LookupTable12to16TestRunner.SetInput("width", width, 0);
    LookupTable12to16TestRunner.SetInput("height", height, 0);
    LookupTable12to16TestRunner.Run(0);
   
    LookupTable12to16CycleCount = LookupTable12to16TestRunner.GetVariableValue(std::string("cycleCount"));
    functionInfo.AddCyclePerPixelInfo((float)(LookupTable12to16CycleCount - 11)/ (float)width);
    
    LookupTable12to16TestRunner.GetOutput("output", 0, width * 2, maxWidth, height, (unsigned char**)dest);
}
