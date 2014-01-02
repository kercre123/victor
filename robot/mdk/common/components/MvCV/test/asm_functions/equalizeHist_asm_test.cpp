#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerEqualizeHist(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int equalizeHistCycleCount;

void equalizeHist_asm_test(unsigned char** in, unsigned char** out, long unsigned int* hist, unsigned int width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerEqualizeHist.Init();
	testRunnerEqualizeHist.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerEqualizeHist.SetInput("hist", (unsigned char*)hist, 256 * 4, 0);
	testRunnerEqualizeHist.SetInput("width", width, 0);
	testRunnerEqualizeHist.Run(0);
	equalizeHistCycleCount = testRunnerEqualizeHist.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(equalizeHistCycleCount - 12)/ (float)width);

	testRunnerEqualizeHist.GetOutput("output", 0, width * 4, maxWidth, 1, out);
	testRunnerEqualizeHist.GetOutput("hist", 0, 256 * 4, maxWidth, 1, (unsigned char**)&hist);
}

