#include "core.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner boxFilterTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int boxFilterCycleCount;

void boxfilter_asm_test(unsigned char** in, unsigned char** out, unsigned long kernelSize, unsigned long normalize, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for linesizes lower than it
	unsigned int padding = 16;
	unsigned int maxWidth = 1920 + padding;

	boxFilterTestRunner.Init();
    boxFilterTestRunner.SetInput("input", in, width + padding, maxWidth, kernelSize, 0);
    boxFilterTestRunner.SetInput("normalize", (unsigned int)normalize, 0);
    boxFilterTestRunner.SetInput("kernelSize", (unsigned int)kernelSize, 0);
    boxFilterTestRunner.SetInput("width", (unsigned int)width, 0);

	boxFilterTestRunner.Run(0);

	boxFilterCycleCount = boxFilterTestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//runnung the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(boxFilterCycleCount - 157)/ (float)width);

	boxFilterTestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
