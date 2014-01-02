#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>
#include "imgproc.h"

TestRunner Laplacian5x5TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Laplacian5x5CycleCount;

void Laplacian5x5_asm_test(unsigned char** in, unsigned char** out, unsigned char width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	Laplacian5x5TestRunner.Init();
	//add padding values at the end of each line
	Laplacian5x5TestRunner.SetInput("input", in, width + padding, maxWidth, 5, 0);
	
	Laplacian5x5TestRunner.SetInput("width", (unsigned char)width, 0);

	Laplacian5x5TestRunner.Run(0);
	
	Laplacian5x5CycleCount = Laplacian5x5TestRunner.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(Laplacian5x5CycleCount - 55)/ (float)width);

	Laplacian5x5TestRunner.GetOutput("output", 0, width, maxWidth, 1, out);
}
