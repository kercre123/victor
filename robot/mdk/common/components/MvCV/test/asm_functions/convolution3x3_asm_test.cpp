#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolution3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolution3x3CycleCount;

void Convolution3x3_asm_test(unsigned char** in, unsigned char** out, half conv[9], unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolution3x3TestRunner.Init();
	//add padding values at the end of each line
	convolution3x3TestRunner.SetInput("input", in, width + padding, maxWidth, 3, 0);
	convolution3x3TestRunner.SetInput("conv", (half*)&conv[0], 9, 0);
	convolution3x3TestRunner.SetInput("width", (unsigned int)width, 0);

	convolution3x3TestRunner.Run(0);
	
//	convolution3x3CycleCount = convolution3x3TestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolution3x3CycleCount - 44)/ (float)width);

	convolution3x3TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
