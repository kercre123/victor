#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolutionSeparable15x15TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolutionSeparable15x15CycleCount;

void ConvolutionSeparable15x15_asm_test(unsigned char** in, unsigned char** out, unsigned char **interm,
		half vconv[15], half hconv[15],  unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 16;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolutionSeparable15x15TestRunner.Init();
	//add padding values at the end of each line
	convolutionSeparable15x15TestRunner.SetInput("input", in, width + padding, maxWidth, 15, 0);
	convolutionSeparable15x15TestRunner.SetInput("interm", interm, width + padding, maxWidth, 1, 0);
	convolutionSeparable15x15TestRunner.SetInput("vconv", vconv, 15, 0);
	convolutionSeparable15x15TestRunner.SetInput("hconv", hconv, 15, 0);
	convolutionSeparable15x15TestRunner.SetInput("width", (unsigned int)width, 0);

	convolutionSeparable15x15TestRunner.Run(0);

	convolutionSeparable15x15CycleCount = convolutionSeparable15x15TestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolutionSeparable15x15CycleCount - 44)/ (float)width);

	convolutionSeparable15x15TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
