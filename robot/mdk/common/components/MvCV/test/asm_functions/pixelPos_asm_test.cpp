#include "core.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner PixelPosTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int PixelPosCycleCount;


void pixelPos_asm_test(unsigned char** srcAddr, unsigned char* maskAddr, unsigned long width, 
	      unsigned char pixelValue, unsigned long* pixelPosition, unsigned char* status)

{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;

	PixelPosTestRunner.Init();
	PixelPosTestRunner.SetInput("input", srcAddr, width, maxWidth, 1, 0);
	PixelPosTestRunner.SetInput("mask", maskAddr, width, 0);
	PixelPosTestRunner.SetInput("width", (unsigned int)width, 0);
	PixelPosTestRunner.SetInput("pixelValue", pixelValue, 0);
	PixelPosTestRunner.SetInput("pixelPosition", (unsigned int)*pixelPosition, 0);
	PixelPosTestRunner.SetInput("status", (unsigned int)*status, 0);
	PixelPosTestRunner.Run(0);
	

	PixelPosCycleCount = PixelPosTestRunner.GetVariableValue("cycleCount");
	//the value substracted from minMaxCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(PixelPosCycleCount - 5) / (float)width);

	PixelPosTestRunner.GetOutput("pixelPosition", 0, pixelPosition);
	PixelPosTestRunner.GetOutput("status", 0, status);
}
