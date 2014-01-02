//medianFilter5x5 kernel test

//Asm function prototype:
//	void medianFilter5x5_asm(u32 widthLine, u8 **outLine, u8 ** inLine);

//Asm test function prototype:
//    	void medianFilter5x5_asm_test(unsigned char width, unsigned char **outLine, unsigned char **inLine);

//C function prototype:
//  	void medianFilter5x5(u32 widthLine, u8 **outLine, u8 ** inLine);

//widthLine	- width of input line
//outLine		- array of pointers for output lines
//inLine 		 - array of pointers to input lines

#include "gtest/gtest.h"
#include "imgproc.h"
#include "medianFilter5x5_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 4

using namespace mvcv;

class medianFilter5x5Test : public ::testing::Test {
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

TEST_F(medianFilter5x5Test, TestUniformInput0Size5x5)
{
	width = 5;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetLines(width, 1, 0);
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestUniformInput255Size5x5)
{
	width = 5;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetLines(width, 1, 255);
	
	for(int i = 0; i < height; i++)
	{
		inLines[i][0] = 0;
		inLines[i][1] = 0;
		inLines[i][width + PADDING / 2] = 0;
		inLines[i][width + PADDING / 2 + 1] = 0;
	}
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
		
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestUniformInput114Size5x5)
{
	width = 5;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 114);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetLines(width, 1, 114);
	
	for(int i = 0; i < height; i++)
	{
		inLines[i][0] = 0;
		inLines[i][1] = 0;
		inLines[i][width + PADDING / 2] = 0;
		inLines[i][width + PADDING / 2 + 1] = 0;
	}
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestPseudoRandomInputSize5x5)
{
	width = 5;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetEmptyLines(width, 1);
	
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
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	aux[0][0] = 2;
	aux[0][1] = 4;
	aux[0][2] = 5;
	aux[0][3] = 3;
	aux[0][4] = 2;
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}


TEST_F(medianFilter5x5Test, TestPseudoRandomInputSize5x8)
{
	width = 8;
	height = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetEmptyLines(width, 1);
	
	inLines[0][2] = 173;
	inLines[0][3] = 180;
	inLines[0][4] = 191;
	inLines[0][5] = 169;
	inLines[0][6] = 169;
	inLines[0][7] = 205;
	inLines[0][8] = 177;
	inLines[0][9] = 200;	
	inLines[1][2] = 200;
	inLines[1][3] = 201;
	inLines[1][4] = 198;
	inLines[1][5] = 197;
	inLines[1][6] = 198;	
	inLines[1][7] = 201;
	inLines[1][8] = 250;
	inLines[1][9] = 255;	
	inLines[2][2] = 201;
	inLines[2][3] = 201;
	inLines[2][4] = 172;
	inLines[2][5] = 188;
	inLines[2][6] = 189;
	inLines[2][7] = 172;
	inLines[2][8] = 160;
	inLines[2][9] = 199;	
	inLines[3][2] = 188;
	inLines[3][3] = 188;
	inLines[3][4] = 188;
	inLines[3][5] = 199;
	inLines[3][6] = 199;
	inLines[3][7] = 199;
	inLines[3][8] = 201;
	inLines[3][9] = 168;	
	inLines[4][2] = 175;
	inLines[4][3] = 176;
	inLines[4][4] = 177;
	inLines[4][5] = 176;
	inLines[4][6] = 175;
	inLines[4][7] = 183;
	inLines[4][8] = 191;
	inLines[4][9] = 180;
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	aux[0][0] = 175;
	aux[0][1] = 180;
	aux[0][2] = 188;
	aux[0][3] = 188;
	aux[0][4] = 189;
	aux[0][5] = 191;
	aux[0][6] = 183;
	aux[0][7] = 172;
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestRandomInputSize5x320)
{
	width = 320;
	height = 5;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestRandomInputSize5x720)
{
	width = 720;
	height = 5;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(medianFilter5x5Test, TestRandomInputSize5x1920)
{
	width = 1920;
	height = 5;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	medianFilter5x5(width, outLinesC, inLinesC);
	medianFilter5x5_asm_test(width + PADDING, outLinesAsm, inLines);
	RecordProperty("CyclePerPixel", medianFilter5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}