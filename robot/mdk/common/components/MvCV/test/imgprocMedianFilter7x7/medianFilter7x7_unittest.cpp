//medianFilter7x7 kernel test

//Asm function prototype:
//	void medianFilter7x7_asm(u32 widthLine, u8 **outLine, u8 ** inLine);

//Asm test function prototype:
//    	void medianFilter7x7_asm_test(unsigned char width, unsigned char **outLine, unsigned char **inLine);

//C function prototype:
//  	void medianFilter7x7(u32 widthLine, u8 **outLine, u8 ** inLine);

//widthLine	- width of input line
//outLine		- array of pointers for output lines
//inLine 		 - array of pointers to input lines

#include "gtest/gtest.h"
#include "imgproc.h"
#include "medianFilter7x7_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 6

using namespace mvcv;

class medianFilter7x7Test : public ::testing::Test {
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
	unsigned char** aux;
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


TEST_F(medianFilter7x7Test, TestUniformInput0Size7x7)
{
	width = 7;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter7x7Test, TestUniformInput255Size7x7)
{
	width = 7;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	for(int i = 0; i < height; i++)
	{
		inLines[i][0] = 0;
		inLines[i][1] = 0;
		inLines[i][2] = 0;
		inLines[i][width + PADDING - 1] = 0;
		inLines[i][width + PADDING - 2] = 0;
		inLines[i][width + PADDING - 3] = 0;	
	}
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter7x7Test, TestUniformInput198Size7x7)
{
	width = 7;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 198);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	for(int i = 0; i < height; i++)
	{
		inLines[i][0] = 0;
		inLines[i][1] = 0;
		inLines[i][2] = 0;
		inLines[i][width + PADDING - 1] = 0;
		inLines[i][width + PADDING - 2] = 0;
		inLines[i][width + PADDING - 3] = 0;	
	}
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter7x7Test, TestPseudoRandomInputSize7x7)
{
	width = 7;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetEmptyLines(width, 1);
	
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
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	aux[0][0] = 1;
	aux[0][1] = 3;
	aux[0][2] = 3;
	aux[0][3] = 4;
	aux[0][4] = 4;
	aux[0][5] = 3;
	aux[0][6] = 0;
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter7x7Test, TestRandomInputSize7x7)
{
	width = 7;
	height = 7;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);
 
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(medianFilter7x7Test, TestRandomInputSize7x640)
{
	width = 640;
	height = 7;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter7x7Test, TestRandomInputSize7x1920)
{
	width = 1920;
	height = 7;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);
	
	medianFilter7x7(width, outLinesC, inLinesC);
	medianFilter7x7_asm_test(width, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter7x7CycleCount / width);
	 
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}