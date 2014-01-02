#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner CornerMinEigenVal_patchedTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int CornerMinEigenVal_patchedCycleCount;

void CornerMinEigenVal_patched_asm_test(unsigned char **input_buffer, unsigned int posy, unsigned char *out_pix, unsigned int width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	CornerMinEigenVal_patchedTestRunner.Init();
	//add padding values at the end of each line
	CornerMinEigenVal_patchedTestRunner.SetInput("input", input_buffer, width + padding, maxWidth, 5, 0);
	CornerMinEigenVal_patchedTestRunner.SetInput("width", (unsigned int)width, 0);
	CornerMinEigenVal_patchedTestRunner.SetInput("posy", (unsigned int)posy, 0);

	CornerMinEigenVal_patchedTestRunner.Run(0);

	CornerMinEigenVal_patchedCycleCount = CornerMinEigenVal_patchedTestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(CornerMinEigenVal_patchedCycleCount - 33));

	CornerMinEigenVal_patchedTestRunner.GetOutput(string("output"), 0, 1, out_pix);
	
}
