//Gauss kernel test
//Asm function prototype:
//     void gauss_asm(u8** in, u8** out, u32 width);
//
//Asm test function prototype:
//	void gauss_asm_test(unsigned char** in, unsigned char** out, unsigned long width);
//
//C function prototype:
//     void gauss(u8** in, u8** out, u32 width)
//	
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// width     - width of input line (min. 8 )
//


#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "gaussKernel_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>
#include <stdlib.h>

using namespace mvcv;
#define PADDING 4
#define DELTA 1

class GaussKernelTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned int width;

	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

 
//1

TEST_F(GaussKernelTest, TestUniformInputLines32)
{
	int width=32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
	
}

//2

TEST_F(GaussKernelTest, TestUniformInputLinesAllZero)
{
	int width=32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
	
}

//3

TEST_F(GaussKernelTest, TestUniformInputLinesAll255)
{
	int width=32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
			
}

//4

TEST_F(GaussKernelTest, TestRandomInputLineLength8)
{
	int width=8;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5,0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
	
}

//5

TEST_F(GaussKernelTest, TestRandomInputLineLength240)
{
	int width=240;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0,255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);
	
	
	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
	
	
			
}

//6

TEST_F(GaussKernelTest, TestRandomInputLineLength320)
{
	int width=320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5,0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width,DELTA);
			
}

//7

TEST_F(GaussKernelTest, TestRandomInputLineLength640)
{
	int width=640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5,0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width,DELTA);
			
}

//8

TEST_F(GaussKernelTest, TestRandomInputLineLength800)
{
	int width=800;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5,0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width,DELTA);
			
}

//9

TEST_F(GaussKernelTest, TestRandomInputLineLength1280)
{
	int width=1280;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5,0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width,DELTA);
			
}


//10

TEST_F(GaussKernelTest, TestEdges)
{
	int i,j;
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 1);
	
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width,1);

	gauss(inLines, outLinesC, width);
	gauss_asm_test(inLines, outLinesAsm,width);
	RecordProperty("CyclePerPixel", gaussCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width,DELTA);
	for(j=0; j<3;j++)
		EXPECT_EQ(outLinesC[0][j], outLinesAsm[0][j]);
	for(j=317; j<320;j++)
		EXPECT_EQ(outLinesC[0][j], outLinesAsm[0][j]);
}
