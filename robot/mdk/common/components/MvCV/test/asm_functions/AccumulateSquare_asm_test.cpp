#include "video.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner AccumulateSquareTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int AccumulateSquareCycleCount;

void AccumulateSquare_asm_test(unsigned char** srcAddr, unsigned char** maskAddr, float** destAddr, unsigned int width, unsigned int height)
{
	
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	AccumulateSquareTestRunner.Init();
	AccumulateSquareTestRunner.SetInput("input", srcAddr, width, maxWidth, height, 0);
	AccumulateSquareTestRunner.SetInput("mask", maskAddr, width, maxWidth, height, 0);
	AccumulateSquareTestRunner.SetInput("output", (unsigned char**)destAddr, width * 4, maxWidth, height, 0);
	AccumulateSquareTestRunner.SetInput("width", width, 0);
	AccumulateSquareTestRunner.SetInput("height", height, 0);

	AccumulateSquareTestRunner.Run(0);
	AccumulateSquareCycleCount = AccumulateSquareTestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(AccumulateSquareCycleCount - 42)/ (float)width);
	AccumulateSquareTestRunner.GetOutput("output", 0, width, maxWidth, 1, destAddr);
}