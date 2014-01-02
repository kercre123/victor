#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"

TestRunner testRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int minMaxCycleCount;

void minMaxKernel_asm_test(unsigned char** in, unsigned int width, unsigned int height,
		unsigned char* minVal, unsigned char* maxVal, unsigned char* maskAddr)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned char* l = in[0];

	testRunner.Init();
	testRunner.SetInput("input", l, width, 0);
	testRunner.SetInput("width", width, 0);
	testRunner.SetInput("height", height, 0);
	testRunner.SetInput("minVal", *minVal, 0);
	testRunner.SetInput("maxVal", *maxVal, 0);
	testRunner.SetInput("mask", maskAddr, width*height, 0);

	testRunner.Run(0);
	minMaxCycleCount = testRunner.GetVariableValue(std::string("cycleCount"));
	//value 5 substracted from minMaxCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(minMaxCycleCount - 5)/ (float)width);

	testRunner.GetOutput("minVal", 0, minVal);
	testRunner.GetOutput("maxVal", 0, maxVal);
}

