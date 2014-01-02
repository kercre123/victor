//Laplacian7x7 kernel test
//Asm function prototype:
//     void Laplacian7x7_asm(u8** in, u8** out, u32 inWidth);
//
//Asm test function prototype:
//     void Laplacian7x7_asm_test(unsigned char** in, unsigned char** out, unsigned long inWidth);
//
//C function prototype:
//     void Laplacian7x7(u8** in, u8** out, u32 inWidth);
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
#include "Laplacian7x7_asm_test.h"
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


#define PADDING 6
#define DELTA 1 //accepted tolerance between C and ASM results

class Laplacian7x7KernelTest : public ::testing::TestWithParam< unsigned long > {
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

TEST_F(Laplacian7x7KernelTest, TestUniformInputLinesWidth8)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


 TEST_F(Laplacian7x7KernelTest, TestUniformInputLinesExpectedValues)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesExp = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	for (int i=0; i < width + PADDING; i ++) {
		inLines[0][i] = 1;
		inLines[1][i] = 1;
		inLines[2][i] = 1;
		inLines[3][i] = 1;
		inLines[4][i] = 1;
		inLines[5][i] = 1;
		inLines[6][i] = 1;
		}
	for (int i=0; i < width; i++) {
		outLinesExp[0][i] = 0;
		}
	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	
	outputChecker.CompareArrays(outLinesC[0], outLinesExp[0], width, DELTA);
	outputChecker.CompareArrays(outLinesAsm[0], outLinesExp[0], width, DELTA);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

 TEST_F(Laplacian7x7KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

		
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
 
TEST_F(Laplacian7x7KernelTest, TestUniformInputLinesAllValues0)
{
	width = 128;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);

	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Laplacian7x7KernelTest, TestUniformInputLinesAllValues255)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Laplacian7x7KernelTest, TestRandomLines)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 7, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	
	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


 TEST_F(Laplacian7x7KernelTest, TestColumnAscendingInputSize7x7)
{
	width = 8;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLines[0][3] = 1;
	inLines[0][4] = 2;
	inLines[0][5] = 7;
	inLines[0][6] = 7;
	inLines[0][7] = 3;
	inLines[0][8] = 2;
	inLines[0][9] = 8;	
	inLines[1][3] = 8;
	inLines[1][4] = 9;
	inLines[1][5] = 3;
	inLines[1][6] = 5;
	inLines[1][7] = 4;
	inLines[1][8] = 7;
	inLines[1][9] = 9;	
	inLines[2][3] = 7;
	inLines[2][4] = 6;
	inLines[2][5] = 5;
	inLines[2][6] = 4;
	inLines[2][7] = 3;
	inLines[2][8] = 2;
	inLines[2][9] = 1;	
	inLines[3][3] = 1;
	inLines[3][4] = 2;
	inLines[3][5] = 3;
	inLines[3][6] = 4;
	inLines[3][7] = 5;
	inLines[3][8] = 6;
	inLines[3][9] = 7;	
	inLines[4][3] = 1;
	inLines[4][4] = 3;
	inLines[4][5] = 5;
	inLines[4][6] = 7;
	inLines[4][7] = 9;
	inLines[4][8] = 0;
	inLines[4][9] = 0;	
	inLines[5][3] = 9;
	inLines[5][4] = 8;
	inLines[5][5] = 7;
	inLines[5][6] = 6;
	inLines[5][7] = 5;
	inLines[5][8] = 4;
	inLines[5][9] = 3;	
	inLines[6][3] = 0;
	inLines[6][4] = 0;
	inLines[6][5] = 3;
	inLines[6][6] = 0;
	inLines[6][7] = 0;
	inLines[6][8] = 9;
	inLines[6][9] = 9;
	

	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
	
}

TEST_F(Laplacian7x7KernelTest, TestRandomLinesMaxWidth)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 7, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);


	
	Laplacian7x7(inLines, outLinesC, width);
	Laplacian7x7_asm_test(inLines, outLinesAsm, width);
	
	RecordProperty("CyclePerPixel", Laplacian7x7CycleCount / width);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
