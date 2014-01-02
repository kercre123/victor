#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner gaussTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int gaussCycleCount;

void gauss_asm_test(unsigned char** in, unsigned char** out, unsigned long width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int padding = 4;
	unsigned int maxWidth = 1920 + padding;

	gaussTestRunner.Init();
	
	gaussTestRunner.SetInput("width", (unsigned int)width, 0);
	gaussTestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	
	gaussTestRunner.Run(0);

	gaussCycleCount = gaussTestRunner.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(gaussCycleCount - 44)/ (float)width);

	gaussTestRunner.GetOutput("output", 0, width, maxWidth, 1, out);
	
}
