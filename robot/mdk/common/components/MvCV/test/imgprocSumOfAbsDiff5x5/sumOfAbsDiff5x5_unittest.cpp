//cvtColorKernelRGBToYUV422 kernel test

//Asm function prototype:
//	 void sumOfAbsDiff5x5_asm(u8** in1, u8** in2, u8** out, u32 width);

//Asm test function prototype:
//    	void sumOfAbsDiff5x5_asm_test(unsigned char **input1, unsigned char **input2, unsigned char **output, unsigned int width);

//C function prototype:
//  	 void sumOfAbsDiff5x5(u8** in1, u8** in2, u8** out, u32 width);

//width	- width of input line
//in1		- first image
//in2		- the second image
//out		- the output line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "sumOfAbsDiff5x5_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 4

using namespace mvcv;

class sumOfAbsDiff5x5Test : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char** inLine1;
	unsigned char** inLine2;
	unsigned char** outLineC;
	unsigned char** outLineAsm;
	unsigned char** outLineExpected;
	unsigned int width;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};


TEST_F(sumOfAbsDiff5x5Test, TestSameInput0)
{
	width = 512 + PADDING;
	inputGen.SelectGenerator("uniform");
	inLine1 = inputGen.GetLines(width, 5, 0);
	inLine2 = inputGen.GetLines(width, 5, 0);
	outLineExpected = inputGen.GetLines(width, 1, 0);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	

	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], width - PADDING, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}


TEST_F(sumOfAbsDiff5x5Test, TestSameInput77)
{
	width = 240 + PADDING;
	inputGen.SelectGenerator("uniform");
	inLine1 = inputGen.GetLines(width, 5, 77);
	inLine2 = inputGen.GetLines(width, 5, 77);
	outLineExpected = inputGen.GetLines(width, 1, 0);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	

	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], width - PADDING, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}

TEST_F(sumOfAbsDiff5x5Test, TestOnePixelDistinct)
{
	width = 720 + PADDING;
	inputGen.SelectGenerator("uniform");
	inLine1 = inputGen.GetLines(width, 5, 215);
	inLine2 = inputGen.GetLines(width, 5, 215);
	outLineExpected = inputGen.GetLines(width, 1, 0);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	
	
	inLine1[2][3] = 15;
	outLineExpected[0][0] = 200;  
	outLineExpected[0][1] = 200;  
	outLineExpected[0][2] = 200;  
	outLineExpected[0][3] = 200;  
		
	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], width - PADDING, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}

TEST_F(sumOfAbsDiff5x5Test, TestPseudoRandomInput)
{
	width =  8 + PADDING;
	inputGen.SelectGenerator("uniform");
	inLine1 = inputGen.GetLines(width, 5, 0);
	inLine2 = inputGen.GetLines(width, 5, 0);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	
	for(int i = 0; i < 5; i++)
	{
		for(int j = 0; j < 5; j++)
		{
			inLine1[i][j] = i * 5 + j + 1;
			inLine2[i][j] = (5 - i) * 5 - j;
		}
	}
	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	EXPECT_EQ(outLineC[0][0], 255);
	EXPECT_EQ(outLineAsm[0][0], 255);
}

TEST_F(sumOfAbsDiff5x5Test, TestRandomInput)
{
	width = 320 + PADDING;
	inputGen.SelectGenerator("random");
	inLine1 = inputGen.GetLines(width, 5, 0, 255);
	inLine2 = inputGen.GetLines(width, 5, 0, 255);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	

	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}

TEST_F(sumOfAbsDiff5x5Test, TestRandomInputMaxWidth)
{
	width = 1920 + PADDING;
	inputGen.SelectGenerator("random");
	inLine1 = inputGen.GetLines(width, 5, 0, 255);
	inLine2 = inputGen.GetLines(width, 5, 0, 255);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	

	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}

TEST_F(sumOfAbsDiff5x5Test, TestRandomInputMinWidth)
{
	width = 8 + PADDING;
	inputGen.SelectGenerator("random");
	inLine1 = inputGen.GetLines(width, 5, 0, 255);
	inLine2 = inputGen.GetLines(width, 5, 0, 255);
	outLineC = inputGen.GetEmptyLines(width, 1);
	outLineAsm = inputGen.GetEmptyLines(width, 1);	

	sumOfAbsDiff5x5(inLine1, inLine2, outLineC, width);
	sumOfAbsDiff5x5_asm_test(inLine1, inLine2, outLineAsm, width);
	RecordProperty("CyclePerPixel", sumOfAbsDiff5x5CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], width - PADDING, 1);
}