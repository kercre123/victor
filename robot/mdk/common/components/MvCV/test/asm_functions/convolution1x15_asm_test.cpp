#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution1x15TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution1x15CycleCount;

void Convolution1x15_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 16;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution1x15TestRunner.Init();
	//add padding values at the end of each line
	convolution1x15TestRunner.SetInput("input", in, width + padding, maxWidth, 1, 0);
	convolution1x15TestRunner.SetInput("conv", conv, 15, 0);
	convolution1x15TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution1x15TestRunner.Run(0);

	convolution1x15CycleCount = convolution1x15TestRunner.GetVariableValue("cycleCount");
	//value 44 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution1x15CycleCount - 44)/ (float)width);

	convolution1x15TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
