#include "half.h"
#include "TestRunner.h"
#include "FunctionInfo.h"
#include "moviDebugDll.h"
#include <cstdio>
#include "imgproc.h"

TestRunner Laplacian3x3TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);
unsigned int Laplacian3x3CycleCount;

void Laplacian3x3_asm_test(unsigned char** in, unsigned char** out, unsigned char width)
{
	FunctionInfo& functionInfo = FunctionInfo::Instance();
	// for calculation of the last values on the line a padding of kenrnel_size - 1 is needed
	const unsigned int padding = 4;
	// maxWidth - the maximum image width for which was allocated space in the board application;
	// it is needed to permit proper memory initialization for line sizes lower than it
	unsigned int maxWidth = 1920 + padding;

	Laplacian3x3TestRunner.Init();
	//add padding values at the end of each line
	Laplacian3x3TestRunner.SetInput("input", in, width + padding, maxWidth, 3, 0);
	
	Laplacian3x3TestRunner.SetInput("width", (unsigned char)width, 0);

	Laplacian3x3TestRunner.Run(0);
	
	Laplacian3x3CycleCount = Laplacian3x3TestRunner.GetVariableValue("cycleCount");
	
	functionInfo.AddCyclePerPixelInfo((float)(Laplacian3x3CycleCount - 55)/ (float)width);

	Laplacian3x3TestRunner.GetOutput("output", 0, width, maxWidth, 1, out);
}
