//Laplacian5x5 kernel test
//Asm function prototype:
//     void Laplacian5x5_asm(u8** in, u8** out, u32 inWidth);
//
//Asm test function prototype:
//     void Lapplacian5x5_asm_test(unsigned char** in, unsigned char** out, unsigned long inWidth);
//
//C function prototype:
//     void Laplacian5x5(u8** in, u8** out, u32 inWidth);
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
#include "Laplacian5x5_asm_test.h"
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


#define PADDING 4
#define DELTA 1 //accepted tolerance between C and ASM results

class Laplacian5x5KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char **inLines;
	unsigned char **outLinesExp;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned char width;
	unsigned char height;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

TEST_F(Laplacian5x5KernelTest, TestUniformInputLinesExpectedValues)
{
	width = 8;
	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesExp = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	for (int i=0; i < width + PADDING; i ++) {
		inLines[0][i] = 1;
		inLines[1][i] = 1;
		inLines[2][i] = 1;
		inLines[3][i] = 1;
		inLines[4][i] = 1;
		}
	for (int i=0; i < width; i++) {
		outLinesExp[0][i] = 1;
		}
	
	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
		
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);	
	
	outputChecker.CompareArrays(outLinesC[0], outLinesExp[0], width, DELTA);
	outputChecker.CompareArrays(outLinesAsm[0], outLinesExp[0], width, DELTA);
	
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Laplacian5x5KernelTest, TestUniformInputLinesWidth8)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
		
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Laplacian5x5KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
		
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Laplacian5x5KernelTest, TestUniformInputLinesAllValues0)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Laplacian5x5KernelTest, TestUniformInputLinesAllValues255)
{
	width = 128;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Laplacian5x5KernelTest, TestRandomLinesWidth640)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

 TEST_F(Laplacian5x5KernelTest, TestColumnAscendingInputSize5x5)
{
	width = 8;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][2] = 7;
	inLines[0][3] = 8;
	inLines[0][4] = 11;
	inLines[0][5] = 5;
	inLines[0][6] = 1;
	inLines[1][2] = 2;
	inLines[1][3] = 3;
	inLines[1][4] = 4;
	inLines[1][5] = 6;
	inLines[1][6] = 8;
	inLines[2][2] = 8;
	inLines[2][3] = 3;
	inLines[2][4] = 9;
	inLines[2][5] = 0;
	inLines[2][6] = 2;
	inLines[3][2] = 2;
	inLines[3][3] = 2;
	inLines[3][4] = 7;
	inLines[3][5] = 7;
	inLines[3][6] = 2;
	inLines[4][2] = 7;
	inLines[4][3] = 7;
	inLines[4][4] = 3;
	inLines[4][5] = 4;
	inLines[4][6] = 5;
	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
	
}

TEST_F(Laplacian5x5KernelTest, TestRandomLinesMaxWidth)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	//inLines = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	Laplacian5x5(inLines, outLinesC, width);
	Laplacian5x5_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian5x5CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
