#include "imgproc.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>


TestRunner cvtColorRGBtoNV21TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int cvtColorRGBtoNV21CycleCount;

void cvtColorRGBtoNV21_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **Yout, unsigned char **UVout,  unsigned int width, unsigned int line)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	cvtColorRGBtoNV21TestRunner.Init();
	cvtColorRGBtoNV21TestRunner.SetInput("inputR", inputR, width, maxWidth, 1, 0);
	cvtColorRGBtoNV21TestRunner.SetInput("inputG", inputG, width, maxWidth, 1, 0);
	cvtColorRGBtoNV21TestRunner.SetInput("inputB", inputB, width, maxWidth, 1, 0);
	cvtColorRGBtoNV21TestRunner.SetInput("width", width, 0);
	cvtColorRGBtoNV21TestRunner.SetInput("line", line, 0);

	cvtColorRGBtoNV21TestRunner.Run(0);
	cvtColorRGBtoNV21CycleCount = cvtColorRGBtoNV21TestRunner.GetVariableValue("cycleCount");
	functionInfo.AddCyclePerPixelInfo((float)(cvtColorRGBtoNV21CycleCount - 23) / (float)width);
	cvtColorRGBtoNV21TestRunner.GetOutput("outputY", 0, width, maxWidth, 1, Yout);
	cvtColorRGBtoNV21TestRunner.GetOutput("outputUV", 0, width, maxWidth, 1, UVout);
}
