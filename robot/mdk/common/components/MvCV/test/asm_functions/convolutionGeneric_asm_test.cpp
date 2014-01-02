#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolutionTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolutionCycleCount;

void Convolution_asm_test(unsigned char** in, unsigned char** out, unsigned long kernelSize, half* conv, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920;
	unsigned int maxKernelSize = 16;

	convolutionTestRunner.Init();
	//add padding values at the end of each line

	convolutionTestRunner.SetInput("input", in, width + padding, maxWidth, kernelSize, 0);
	convolutionTestRunner.SetInput("kernelSize", (unsigned int)kernelSize, 0);
	convolutionTestRunner.SetInput("convMatrix", conv, kernelSize * kernelSize, 0);
	convolutionTestRunner.SetInput("width", (unsigned int)width, 0);

	convolutionTestRunner.Run(0);

	convolutionCycleCount = convolutionTestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolutionCycleCount - 190)/ (float)width);

	convolutionTestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
