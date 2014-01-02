#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner boxFilter7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int boxFilter7x7CycleCount;

void boxfilter7x7_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	boxFilter7x7TestRunner.Init();
	//add padding values at the end of each line
	boxFilter7x7TestRunner.SetInput("input", in, width + padding, maxWidth, 7, 0);
	boxFilter7x7TestRunner.SetInput("normalize", (unsigned int)normalize, 0);
	boxFilter7x7TestRunner.SetInput("width", (unsigned int)width, 0);

	boxFilter7x7TestRunner.Run(0);

	boxFilter7x7CycleCount = boxFilter7x7TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(boxFilter7x7CycleCount - 157)/ (float)width);

	if(normalize != 1)
	{
		//for unnormalized variant the output values are 16bits, so the output size is width * 2
		boxFilter7x7TestRunner.GetOutput(string("output"), 0, width * 2, maxWidth, 1, out);
	}
	else {
		boxFilter7x7TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
	}
}
