#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner boxFilter9x9TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int boxFilter9x9CycleCount;

void boxfilter9x9_asm_test(unsigned char** in, unsigned char** out, unsigned char normalize, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	boxFilter9x9TestRunner.Init();
	//add padding values at the end of each line
	boxFilter9x9TestRunner.SetInput("input", in, width + padding, maxWidth, 9, 0);
	boxFilter9x9TestRunner.SetInput("normalize", normalize, 0);
	boxFilter9x9TestRunner.SetInput("width", width, 0);

	boxFilter9x9TestRunner.Run(0);

	boxFilter9x9CycleCount = boxFilter9x9TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(boxFilter9x9CycleCount - 157)/ (float)width);

	if(normalize != 1)
	{
		//for unnormalized variant the output values are 16bits, so the output size is width * 2
		boxFilter9x9TestRunner.GetOutput(string("output"), 0, width * 2, maxWidth, 1, out);
	}
	else {
		boxFilter9x9TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
	}
}
