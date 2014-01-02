#include "core.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner boxFilter3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int boxFilter3x3CycleCount;

void boxfilter3x3_asm_test(unsigned char** in, unsigned char** out, unsigned long normalize, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// to calculate the last values on the line a padding of kernel_size - 1 is needed
	const unsigned int padding = 16; // need 1 , but for allign reason we use 8
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for linesizes lower than it
	unsigned int maxWidth = 1920 + 16;

	boxFilter3x3TestRunner.Init();
	boxFilter3x3TestRunner.SetInput("input", in, width + padding, maxWidth, 3, 0);
    boxFilter3x3TestRunner.SetInput("normalize", (unsigned int)normalize, 0);
	boxFilter3x3TestRunner.SetInput("width", (unsigned int)width, 0);

	boxFilter3x3TestRunner.Run(0);

	boxFilter3x3CycleCount = boxFilter3x3TestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//runnung the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(boxFilter3x3CycleCount - 157)/ (float)width);

	if(normalize != 1)
	{
		//for unnormalized variant the output values are 16bits, so the output size is width * 2
		boxFilter3x3TestRunner.GetOutput(string("output"), 0, width * 2, maxWidth, 1, out);
	}
	else {
		boxFilter3x3TestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
	}

}
