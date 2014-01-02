//medianFilter3x3 kernel test

//Asm function prototype:
//	void medianFilter3x3_asm(u32 widthLine, u8 **outLine, u8 ** inLine);

//Asm test function prototype:
//    	void medianFilter3x3_asm_test(unsigned char width, unsigned char **outLine, unsigned char **inLine);

//C function prototype:
//  	void medianFilter3x3(u32 widthLine, u8 **outLine, u8 ** inLine);

//widthLine	- width of input line
//outLine		- array of pointers for output lines
//inLine 		 - array of pointers to input lines

#include "gtest/gtest.h"
#include "imgproc.h"
#include "medianFilter3x3_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 2

using namespace mvcv;

class medianFilter3x3Test : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char** inLines;
	unsigned char** outLinesC;
	unsigned char** outLinesAsm;
	unsigned char** inLinesC;
	unsigned int width;
	unsigned int height;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(medianFilter3x3Test, TestUniformInput0Size3x3)
{
	width = 3;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestUniformInput255Size3x3)
{
	width = 3;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestUniformInputRandomValueSize3x3)
{
	width = 3;
	height = 3;
	inputGen.SelectGenerator("uniform");
	int value = randomGen.GenerateUInt(0, 255, 1);
	inLines = inputGen.GetLines(width + PADDING, height, value);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestColumnAscendingInputSize3x3)
{
	width = 3;
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
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestRowDescendingInputSize3x3)
{
	width = 3;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][1] = 119;
	inLines[0][2] = 118;
	inLines[0][3] = 117;
	inLines[1][1] = 116;
	inLines[1][2] = 115;
	inLines[1][3] = 114;
	inLines[2][1] = 113;
	inLines[2][2] = 112;
	inLines[2][3] = 111;
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestPseudoRandomInputSize3x4)
{
	width = 4;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][1] = 78;
	inLines[0][2] = 29;
	inLines[0][3] = 63;
	inLines[0][4] = 59;
	inLines[1][1] = 21;
	inLines[1][2] = 64;
	inLines[1][3] = 73;
	inLines[1][4] = 10;
	inLines[2][1] = 42;
	inLines[2][2] = 51;
	inLines[2][3] = 76;
	inLines[2][4] = 38;
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestPseudoRandomInputSize3x5)
{
	width = 5;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);


	inLines[0][1] = 21;
	inLines[0][2] = 35;
	inLines[0][3] = 13;
	inLines[0][4] = 11;
	inLines[0][5] = 16;
	inLines[1][1] = 24;
	inLines[1][2] = 19;
	inLines[1][3] = 81;
	inLines[1][4] = 72;
	inLines[1][5] = 33;
	inLines[2][1] = 34;
	inLines[2][2] = 22;
	inLines[2][3] = 12;
	inLines[2][4] = 14;
	inLines[2][5] = 17;
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestPseudoRandomInputSize3x8)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][1] = 2;
	inLines[0][2] = 1;
	inLines[0][3] = 7;
	inLines[0][4] = 5;
	inLines[0][5] = 8;
	inLines[0][6] = 8;
	inLines[0][7] = 6;
	inLines[0][8] = 8;
	inLines[1][1] = 7;
	inLines[1][2] = 5;
	inLines[1][3] = 3;
	inLines[1][4] = 2;
	inLines[1][5] = 2;
	inLines[1][6] = 4;
	inLines[1][7] = 4;
	inLines[1][8] = 9;	
	inLines[2][1] = 1;
	inLines[2][2] = 2;
	inLines[2][3] = 3;
	inLines[2][4] = 5;
	inLines[2][5] = 7;
	inLines[2][6] = 7;
	inLines[2][7] = 2;
	inLines[2][8] = 2;
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestPseudoRandomInputSize3x1920)
{
	width = 1920;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLines[0][1] = 2;
	inLines[0][2] = 1;
	inLines[0][3] = 7;
	inLines[1][1] = 5;
	inLines[1][2] = 8;
	inLines[1][3] = 8;
	inLines[2][1] = 6;
	inLines[2][2] = 8;	
	inLines[2][3] = 7;
	
	inLines[0][466] = 77;
	inLines[0][467] = 77;
	inLines[0][468] = 77;
	inLines[1][466] = 77;
	inLines[1][467] = 77;
	inLines[1][468] = 77;
	inLines[2][466] = 77;
	inLines[2][467] = 77;	
	inLines[2][468] = 77;
	
	inLines[0][1918] = 72;
	inLines[0][1919] = 79;
	inLines[1][1918] = 27;
	inLines[1][1919] = 87;
	inLines[2][1918] = 87;
	inLines[2][1919] = 17;		
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter3x3Test, TestRandomInputSize3x720)
{
	width = 720;
	height = 3;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	for(int i = 0; i < height; i++)
	{
		inLines[i][0] = 0;
		inLines[i][width + 1] = 0;
	}
	
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	medianFilter3x3(width, outLinesC, inLinesC);
	medianFilter3x3_asm_test(width + (2 * PADDING), outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter3x3CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}