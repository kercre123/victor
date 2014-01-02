#include "core.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner thresholdKernelTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int thresholdKernelCycleCount;

void thresholdKernel_asm_test(unsigned char** in, unsigned char** out, unsigned int width, unsigned int height, unsigned char thresh, unsigned int thresh_type)
{
	
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned char* o = out[0];	
	unsigned int maxWidth = 1920;
	
	thresholdKernelTestRunner.Init();

	thresholdKernelTestRunner.SetInput("input", in, width, maxWidth, height, 0);
	thresholdKernelTestRunner.SetInput("width", width, 0);
	thresholdKernelTestRunner.SetInput("height", height, 0);
	thresholdKernelTestRunner.SetInput("thresh", thresh, 0);
	thresholdKernelTestRunner.SetInput("thresh_type", thresh_type, 0);
	
	thresholdKernelTestRunner.Run(0);
	thresholdKernelCycleCount = thresholdKernelTestRunner.GetVariableValue("cycleCount");
	//value substracted from minMaxCycleCount is the value of measured cycles
	//for computations not included in thresholdKernel_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(thresholdKernelCycleCount - 46)/ (float)width);
	thresholdKernelTestRunner.GetOutput("output", 0, width, maxWidth, height, out);
}
