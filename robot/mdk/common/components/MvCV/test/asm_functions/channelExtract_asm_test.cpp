#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner channelExtractTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int channelExtractCycleCount;

void channelExtract_asm_test(unsigned char** in, unsigned char** out,unsigned long width,unsigned char plane)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920;

	channelExtractTestRunner.Init();
	//add padding values at the end of each line
	channelExtractTestRunner.SetInput("input", in, width, maxWidth, 1, 0);
	channelExtractTestRunner.SetInput("plane", (unsigned int)plane, 0);
	channelExtractTestRunner.SetInput("width", (unsigned int)width, 0);

	channelExtractTestRunner.Run(0);

	channelExtractCycleCount = channelExtractTestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(channelExtractCycleCount - 45)/ (float)width);

	channelExtractTestRunner.GetOutput(string("output"), 0, width / 3, maxWidth, 1, out);
}
