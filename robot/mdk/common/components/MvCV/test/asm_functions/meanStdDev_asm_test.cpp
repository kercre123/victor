#include "core.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner meanstddevTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int meanstddevCycleCount;

void meanstddev_asm_test(unsigned char** in, float *mean, float *stdev, unsigned int width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	meanstddevTestRunner.Init();
	meanstddevTestRunner.SetInput("input", in, width, maxWidth, 1, 0);
	meanstddevTestRunner.SetInput("width", width, 0);
	meanstddevTestRunner.SetInput("mean", mean, 1, 0);
	meanstddevTestRunner.SetInput("stdev", stdev, 1, 0);
	meanstddevTestRunner.Run(0);
	meanstddevCycleCount = meanstddevTestRunner.GetVariableValue(std::string("cycleCount"));
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(meanstddevCycleCount - 7)/ (float)width);
	meanstddevTestRunner.GetOutput("mean", 0, 1, mean);
	meanstddevTestRunner.GetOutput("stdev", 0, 1, stdev);
}
