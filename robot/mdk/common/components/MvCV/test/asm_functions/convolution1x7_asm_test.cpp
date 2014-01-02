#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution1x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution1x7CycleCount;

void Convolution1x7_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution1x7TestRunner.Init();
	//add padding values at the end of each line
	convolution1x7TestRunner.SetInput("input", in, width + padding, maxWidth, 1, 0);
	convolution1x7TestRunner.SetInput("conv", conv, 7, 0);
	convolution1x7TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution1x7TestRunner.Run(0);

	convolution1x7CycleCount = convolution1x7TestRunner.GetVariableValue("cycleCount");
	//value 44 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution1x7CycleCount - 44)/ (float)width);

	convolution1x7TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
