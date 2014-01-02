#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner warpAffineBilinearTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int warpAffineBilinearCycleCount;

void WarpAffineTransformBilinear_asm_test(unsigned char** in, unsigned char** out, unsigned int in_width, unsigned int in_height,
		unsigned int out_width, unsigned int out_height, unsigned int in_stride, unsigned int out_stride,
		unsigned char* fill_color, half imat[2][3])
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	warpAffineBilinearTestRunner.Init();

	unsigned char* input = (unsigned char*)&in[0][0];
	warpAffineBilinearTestRunner.SetInput("input", input, in_width * in_height, 0);
	warpAffineBilinearTestRunner.SetInput("in_width", in_width, 0);
	warpAffineBilinearTestRunner.SetInput("in_height", in_height, 0);
	warpAffineBilinearTestRunner.SetInput("out_width", out_width, 0);
	warpAffineBilinearTestRunner.SetInput("out_height", out_height, 0);
	warpAffineBilinearTestRunner.SetInput("in_stride", in_stride, 0);
	warpAffineBilinearTestRunner.SetInput("out_stride", out_stride, 0);
	warpAffineBilinearTestRunner.SetInput("fill_color", fill_color, 1, 0);
	warpAffineBilinearTestRunner.SetInput("imat", (unsigned char*)&imat[0], 3*2*2, 0);

	warpAffineBilinearTestRunner.Run(0);

	warpAffineBilinearCycleCount = warpAffineBilinearTestRunner.GetVariableValue("cycleCount");
	//the value substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(warpAffineBilinearCycleCount - 61)/ (float)in_width);

	warpAffineBilinearTestRunner.GetOutput(string("output"), 0, out_width * out_height, *out);
}
