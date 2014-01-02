#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution5x5CycleCount;

void Convolution5x5_asm_test(unsigned char** in, unsigned char** out, half conv[25], unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution5x5TestRunner.Init();
	//add padding values at the end of each line
	convolution5x5TestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	convolution5x5TestRunner.SetInput("conv", (half*)&conv[0], 25, 0);
	convolution5x5TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution5x5TestRunner.Run(0);

	convolution5x5CycleCount = convolution5x5TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution5x5CycleCount - 44)/ (float)width);

	convolution5x5TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
