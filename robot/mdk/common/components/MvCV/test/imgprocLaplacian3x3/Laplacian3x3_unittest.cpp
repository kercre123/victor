//Laplacian3x3 kernel test
//Asm function prototype:
//     void Laplacian3x3_asm(u8** in, u8** out, u32 inWidth);
//
//Asm test function prototype:
//     void Laplacian3x3_asm_test(unsigned char** in, unsigned char** out, unsigned long inWidth);
//
//C function prototype:
//     void Laplacian3x3(u8** in, u8** out, u32 inWidth);
//
//Parameter description:
// in         - array of pointers to input lines
// out        - array of pointers to output lines
//
// inWidth    - width of input line
//
// each line needs a padding o kernel_size - 1 values at the end; the value of the padding is equal
// with the last value of line without padding

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "Laplacian3x3_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>
#include "half.h"

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;


#define PADDING 2
#define DELTA 1 //accepted tolerance between C and ASM results

class Laplacian3x3KernelTest : public ::testing::TestWithParam< unsigned long > {
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
	unsigned char width;
	unsigned char height;
	unsigned char** outLinesExp;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

TEST_F(Laplacian3x3KernelTest, TestUniformInputLinesWidth8)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width+PADDING);

	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


 TEST_F(Laplacian3x3KernelTest, TestUniformInputLinesExpectedValues)
{
	width = 8;
	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesExp = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	for (int i=0; i < width + PADDING - 2; i +=2) {
		inLines[0][i] = 1;
		inLines[0][i+1] = 1;
		inLines[1][i] = 1;
		inLines[1][i+1] = 4;
		inLines[2][i] = 1;
		inLines[2][i+1] = 1;
	}
	for (int i=0; i < width; i+=2) {
		outLinesExp[0][i] = 12;
		outLinesExp[0][i+1] = 0;
	}
	
	
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesExp[0], width, DELTA);
	outputChecker.CompareArrays(outLinesAsm[0], outLinesExp[0], width, DELTA);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
} 

 TEST_F(Laplacian3x3KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Laplacian3x3KernelTest, TestUniformInputLinesAllValues0)
{
	width = 128;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Laplacian3x3KernelTest, TestUniformInputLinesAllValues255)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Laplacian3x3KernelTest, TestRandomLines)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

 TEST_F(Laplacian3x3KernelTest, TestColumnAscendingInputSize3x3)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][1] = 101;
	inLines[0][2] = 104;
	inLines[0][3] = 107;
	inLines[1][1] = 102;
	inLines[1][2] = 105;
	inLines[1][3] = 108;
	inLines[2][1] = 103;
	inLines[2][2] = 106;
	inLines[2][3] = 109;
	

	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
	
}

TEST_F(Laplacian3x3KernelTest, TestRandomLinesMaxWidth)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	Laplacian3x3(inLines, outLinesC, width);
	Laplacian3x3_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian3x3CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
