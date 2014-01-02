#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>


TestRunner cvtColorKernelYUV422ToRGBTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int cvtColorKernelYUV422ToRGBCycleCount;

void cvtColorKernelYUV422ToRGB_asm_test(unsigned char **input, unsigned char **rOut, unsigned char **gOut, unsigned char **bOut, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorKernelYUV422ToRGBTestRunner.Init();
	cvtColorKernelYUV422ToRGBTestRunner.SetInput("input", input, 2 * width, 2 * maxWidth, 1, 0);
	cvtColorKernelYUV422ToRGBTestRunner.SetInput("width", width, 0);

	cvtColorKernelYUV422ToRGBTestRunner.Run(0);
	cvtColorKernelYUV422ToRGBCycleCount = cvtColorKernelYUV422ToRGBTestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorKernelYUV422ToRGBCycleCount - 18) / (float)width);
	cvtColorKernelYUV422ToRGBTestRunner.GetOutput("outputR", 0, width, maxWidth, 1, rOut);
	cvtColorKernelYUV422ToRGBTestRunner.GetOutput("outputG", 0, width, maxWidth, 1, gOut);
	cvtColorKernelYUV422ToRGBTestRunner.GetOutput("outputB", 0, width, maxWidth, 1, bOut);
}
