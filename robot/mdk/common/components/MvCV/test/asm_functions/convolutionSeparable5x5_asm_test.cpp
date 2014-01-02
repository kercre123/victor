#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolutionSeparable5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolutionSeparable5x5CycleCount;

void ConvolutionSeparable5x5_asm_test(unsigned char** in, unsigned char** out, unsigned char **interm,
		half vconv[5], half hconv[5],  unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolutionSeparable5x5TestRunner.Init();
	//add padding values at the end of each line
	convolutionSeparable5x5TestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	convolutionSeparable5x5TestRunner.SetInput("interm", interm, width + padding, maxWidth, 1, 0);
	convolutionSeparable5x5TestRunner.SetInput("vconv", vconv, 5, 0);
	convolutionSeparable5x5TestRunner.SetInput("hconv", hconv, 5, 0);
	convolutionSeparable5x5TestRunner.SetInput("width", (unsigned int)width, 0);

	convolutionSeparable5x5TestRunner.Run(0);

	convolutionSeparable5x5CycleCount = convolutionSeparable5x5TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolutionSeparable5x5CycleCount - 44)/ (float)width);

	convolutionSeparable5x5TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
