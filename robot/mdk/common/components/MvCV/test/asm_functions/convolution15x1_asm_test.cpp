#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution15x1TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution15x1CycleCount;

void Convolution15x1_asm_test(unsigned char** in, unsigned char** out, half* conv, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920;

	convolution15x1TestRunner.Init();

	convolution15x1TestRunner.SetInput("input", in, width, maxWidth, 15, 0);
	convolution15x1TestRunner.SetInput("conv", conv, 15, 0);
	convolution15x1TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution15x1TestRunner.Run(0);

	convolution15x1CycleCount = convolution15x1TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution15x1CycleCount - 44)/ (float)width);

	convolution15x1TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
