#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner convolutionSeparable7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int convolutionSeparable7x7CycleCount;

void ConvolutionSeparable7x7_asm_test(unsigned char** in, unsigned char** out, unsigned char **interm,
		half vconv[7], half hconv[7],  unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	convolutionSeparable7x7TestRunner.Init();
	//add padding values at the end of each line
	convolutionSeparable7x7TestRunner.SetInput("input", in, width + padding, maxWidth, 7, 0);
	convolutionSeparable7x7TestRunner.SetInput("interm", interm, width + padding, maxWidth, 1, 0);
	convolutionSeparable7x7TestRunner.SetInput("vconv", vconv, 7, 0);
	convolutionSeparable7x7TestRunner.SetInput("hconv", hconv, 7, 0);
	convolutionSeparable7x7TestRunner.SetInput("width", (unsigned int)width, 0);

	convolutionSeparable7x7TestRunner.Run(0);

	convolutionSeparable7x7CycleCount = convolutionSeparable7x7TestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(convolutionSeparable7x7CycleCount - 44)/ (float)width);

	convolutionSeparable7x7TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
}
