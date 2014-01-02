#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner boxFilter5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int boxFilter5x5CycleCount;

void boxfilter5x5_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	boxFilter5x5TestRunner.Init();
	//add padding values at the end of each line
	boxFilter5x5TestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	boxFilter5x5TestRunner.SetInput("normalize", (unsigned int)normalize, 0);
	boxFilter5x5TestRunner.SetInput("width", (unsigned int)width, 0);

	boxFilter5x5TestRunner.Run(0);

	boxFilter5x5CycleCount = boxFilter5x5TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(boxFilter5x5CycleCount - 157)/ (float)width);

	if(normalize != 1)
	{
		//for unnormalized variant the output values are 16bits, so the output size is width * 2
		boxFilter5x5TestRunner.GetOutput(string("output"), 0, width * 2, maxWidth, 1, out);
	}
	else {
		boxFilter5x5TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
	}
}
