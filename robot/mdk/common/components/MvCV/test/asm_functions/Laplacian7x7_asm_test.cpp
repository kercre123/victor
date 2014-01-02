#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>
#include "imgproc.h"

TestRunner Laplacian7x7TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Laplacian7x7CycleCount;

void Laplacian7x7_asm_test(unsigned char** in, unsigned char** out, unsigned char width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 8;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	Laplacian7x7TestRunner.Init();
	//add padding values at the end of each line
	Laplacian7x7TestRunner.SetInput("input", in, width + padding, maxWidth, 7, 0);
	
	Laplacian7x7TestRunner.SetInput("width", (unsigned char)width, 0);

	Laplacian7x7TestRunner.Run(0);
	
	Laplacian7x7CycleCount = Laplacian7x7TestRunner.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(Laplacian7x7CycleCount - 55)/ (float)width);

	Laplacian7x7TestRunner.GetOutput("output", 0, width, maxWidth, 1, out);
}
