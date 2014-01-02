#include "feature.h"
#include "TestRunner.h"
#include "RandomGenerator.h"
#include "moviDebugDll.h"
#include "FunctionInfo.h"
#include <cstdio>

TestRunner fast9TestRunner(APP_PATH, APP_ELFPATH, DBG_INTERFACE);

unsigned int fast9CycleCount;

void fast9_asm_test(u8 **inlines, u8* score, unsigned int* base, u32 posy, u32 tresh, u32 width)
{

	FunctionInfo& functionInfo = FunctionInfo::Instance();
	unsigned int maxWidth = 1920;
	fast9TestRunner.Init();
	fast9TestRunner.SetInput("input", inlines, width, maxWidth, 7, 0);
	fast9TestRunner.SetInput("width", (unsigned int) width, 0);
	fast9TestRunner.SetInput("tresh",(unsigned int) tresh, 0);
	fast9TestRunner.SetInput("posy",(unsigned int) posy, 0);

	DllMoviDebugParseString("t s0");
	//DllMoviDebugParseString("get base 20");
	printf("\n");
	//DllMoviDebugParseString("get score 20");

	fast9TestRunner.Run(0);
	fast9CycleCount = fast9TestRunner.GetVariableValue(std::string("cycleCount"));
	functionInfo.AddCyclePerPixelInfo((float)(fast9CycleCount - 5)/ (float)width);

	fast9TestRunner.GetOutput("base", 0, base);
//	DllMoviDebugParseString("t s0");
//	DllMoviDebugParseString("get base 20");
//	printf("\n");
//	DllMoviDebugParseString("get score 20");
	
	fast9TestRunner.GetOutput("score", 0, *base, score);
	fast9TestRunner.GetOutput("base",0, (*base +1)*4,(u8*) base);
}
