#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner absoluteDiffTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int AbsoluteDiffCycleCount;

void AbsoluteDiff_asm_test(unsigned char** in1, unsigned char** in2, unsigned char** out, unsigned int width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	absoluteDiffTestRunner.Init();
	absoluteDiffTestRunner.SetInput("input1", in1, width, maxWidth, 1, 0);
	absoluteDiffTestRunner.SetInput("input2", in2, width, maxWidth, 1, 0);
	absoluteDiffTestRunner.SetInput("width", width, 0);

	absoluteDiffTestRunner.Run(0);

	AbsoluteDiffCycleCount = absoluteDiffTestRunner.GetVariableValue(std::string("cycleCount"));
	functionInfo.AddCyclePerPixelInfo((float)(AbsoluteDiffCycleCount - 5)/ (float)width);

	
	absoluteDiffTestRunner.GetOutput("output", 0, width, 1920, 1, out);
}
