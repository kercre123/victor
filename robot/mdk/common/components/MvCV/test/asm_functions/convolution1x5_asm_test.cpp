#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution1x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution1x5CycleCount;

void Convolution1x5_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution1x5TestRunner.Init();
	//add padding values at the end of each line
	convolution1x5TestRunner.SetInput("input", in, width + padding, maxWidth, 1, 0);
	convolution1x5TestRunner.SetInput("conv", conv, 5, 0);
	convolution1x5TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution1x5TestRunner.Run(0);

	convolution1x5CycleCount = convolution1x5TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution1x5CycleCount - 44)/ (float)width);

	convolution1x5TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
