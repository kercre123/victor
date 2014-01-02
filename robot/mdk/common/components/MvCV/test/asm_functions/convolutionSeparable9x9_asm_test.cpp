#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolutionSeparable9x9TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolutionSeparable9x9CycleCount;

void ConvolutionSeparable9x9_asm_test(unsigned char** in, unsigned char** out, unsigned char **interm,
		half vconv[9], half hconv[9],  unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolutionSeparable9x9TestRunner.Init();
	//add padding values at the end of each line
	convolutionSeparable9x9TestRunner.SetInput("input", in, width + padding, maxWidth, 9, 0);
	convolutionSeparable9x9TestRunner.SetInput("interm", interm, width + padding, maxWidth, 1, 0);
	convolutionSeparable9x9TestRunner.SetInput("vconv", vconv, 9, 0);
	convolutionSeparable9x9TestRunner.SetInput("hconv", hconv, 9, 0);
	convolutionSeparable9x9TestRunner.SetInput("width", (unsigned int)width, 0);

	convolutionSeparable9x9TestRunner.Run(0);

	convolutionSeparable9x9CycleCount = convolutionSeparable9x9TestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolutionSeparable9x9CycleCount - 44)/ (float)width);

	convolutionSeparable9x9TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
