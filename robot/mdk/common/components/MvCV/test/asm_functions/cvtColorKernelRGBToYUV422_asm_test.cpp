#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>


TestRunner cvtColorKernelRGBToYUV422TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int cvtColorKernelRGBToYUV422CycleCount;

void cvtColorKernelRGBToYUV422_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **output, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorKernelRGBToYUV422TestRunner.Init();
	cvtColorKernelRGBToYUV422TestRunner.SetInput("inputR", inputR, width, maxWidth, 1, 0);
	cvtColorKernelRGBToYUV422TestRunner.SetInput("inputG", inputG, width, maxWidth, 1, 0);
	cvtColorKernelRGBToYUV422TestRunner.SetInput("inputB", inputB, width, maxWidth, 1, 0);
	cvtColorKernelRGBToYUV422TestRunner.SetInput("width", width, 0);

	cvtColorKernelRGBToYUV422TestRunner.Run(0);
	
	cvtColorKernelRGBToYUV422CycleCount = cvtColorKernelRGBToYUV422TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorKernelRGBToYUV422CycleCount - 18) / (float)width);	
	cvtColorKernelRGBToYUV422TestRunner.GetOutput("output", 0, 2 * width, 2 * maxWidth, 1, output);

}
