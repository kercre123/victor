#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>


TestRunner cvtColorNV21toRGBTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int cvtColorNV21toRGBCycleCount;
			 

void cvtColorNV21toRGB_asm_test(unsigned char **inputY,unsigned char **inputUV, unsigned char **rOut, unsigned char **gOut, unsigned char **bOut, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorNV21toRGBTestRunner.Init();
	cvtColorNV21toRGBTestRunner.SetInput("inputY", inputY, width, maxWidth, 1, 0);
	cvtColorNV21toRGBTestRunner.SetInput("inputUV", inputUV, width, maxWidth, 1, 0);
	cvtColorNV21toRGBTestRunner.SetInput("width", width, 0);

	cvtColorNV21toRGBTestRunner.Run(0);
	cvtColorNV21toRGBCycleCount = cvtColorNV21toRGBTestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorNV21toRGBCycleCount - 23) / (float)width);
	cvtColorNV21toRGBTestRunner.GetOutput("outputR", 0, width, maxWidth, 1, rOut);
	cvtColorNV21toRGBTestRunner.GetOutput("outputG", 0, width, maxWidth, 1, gOut);
	cvtColorNV21toRGBTestRunner.GetOutput("outputB", 0, width, maxWidth, 1, bOut);
}
