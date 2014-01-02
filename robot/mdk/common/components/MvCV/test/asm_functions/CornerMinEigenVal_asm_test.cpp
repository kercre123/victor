#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>

TestRunner CornerMinEigenValTestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int CornerMinEigenValCycleCount;

void CornerMinEigenVal_asm_test(unsigned char** in, unsigned char** out, unsigned long width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	CornerMinEigenValTestRunner.Init();
	//add padding values at the end of each line
	CornerMinEigenValTestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	CornerMinEigenValTestRunner.SetInput("width", (unsigned int)width, 0);

	CornerMinEigenValTestRunner.Run(0);

	CornerMinEigenValCycleCount = CornerMinEigenValTestRunner.GetVariableValue("cycleCount");
	//value 157 substracted from boxFilterCycleCount is the value of measured cycles
	//for computations not included in boxfilter_asm function. It was obtained by
	//running the application without the function call
	functionInfo.AddCyclePerPixelInfo((float)(CornerMinEigenValCycleCount - 55)/ (float)width);

	CornerMinEigenValTestRunner.GetOutput(string("output"), 0, width, maxWidth, 1, out);
	
}
