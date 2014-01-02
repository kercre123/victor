#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution11x11TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution11x11CycleCount;

void Convolution11x11_asm_test(unsigned char** in, unsigned char** out, half conv[121], unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 16;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution11x11TestRunner.Init();
	//add padding values at the end of each line
	convolution11x11TestRunner.SetInput("input", in, width + padding, maxWidth, 11, 0);
	convolution11x11TestRunner.SetInput("conv", (half*)&conv[0], 121, 0);
	convolution11x11TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution11x11TestRunner.Run(0);

	convolution11x11CycleCount = convolution11x11TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution11x11CycleCount - 44)/ (float)width);

	convolution11x11TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
