#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner testRunnerHistogram(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int histogramCycleCount;

void histogram_asm_test(unsigned char** in, long unsigned int* hist, unsigned int width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();

	unsigned int maxWidth = 1920;

	testRunnerHistogram.Init();
	testRunnerHistogram.SetInput("input", in, width, maxWidth, 1, 0);
	testRunnerHistogram.SetInput("hist", (unsigned char*)hist, 256 * 4, 0);
	testRunnerHistogram.SetInput("width", width, 0);
	testRunnerHistogram.Run(0);
	histogramCycleCount = testRunnerHistogram.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(histogramCycleCount - 7)/ (float)width);

	testRunnerHistogram.GetOutput("hist", 0, 256 * 4, maxWidth, 1, (unsigned char**)&hist);
}

