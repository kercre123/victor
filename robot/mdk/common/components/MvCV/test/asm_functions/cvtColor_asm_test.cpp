#include "imgproc.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner cvtColorTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int cvtColorCycleCount;

void YUVtoRGB_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorTestRunner.Init();

	cvtColorTestRunner.SetInput("width", width, 0);
	cvtColorTestRunner.SetInput("code", (unsigned int)1, 0);

	cvtColorTestRunner.SetInput("y", y, width*2, 0);
	cvtColorTestRunner.SetInput("u", u, width*2/4, 0);
	cvtColorTestRunner.SetInput("v", v, width*2/4, 0);

	cvtColorTestRunner.Run(0);

	cvtColorCycleCount = cvtColorTestRunner.GetVariableValue(std::string("cycleCount"));
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorCycleCount - 5)/ (float)(2 * width));


	cvtColorTestRunner.GetOutput("rgb", 0, width*6 , 1920, 1, &rgb);

}
void RGBtoYUV_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorTestRunner.Init();

	cvtColorTestRunner.SetInput("width", width, 0);
	cvtColorTestRunner.SetInput("code", (unsigned int)0, 0);

	cvtColorTestRunner.SetInput("rgb", rgb, width*2*3, 0);

	cvtColorTestRunner.Run(0);

	cvtColorCycleCount = cvtColorTestRunner.GetVariableValue(std::string("cycleCount"));
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorCycleCount - 5)/ (float)(2 * width));

	cvtColorTestRunner.GetOutput("y", 0, width*2, 1920, 1, &y);
	cvtColorTestRunner.GetOutput("u", 0, width*2/4, 1920, 1, &u);
	cvtColorTestRunner.GetOutput("v", 0, width*2/4, 1920, 1, &v);
}
