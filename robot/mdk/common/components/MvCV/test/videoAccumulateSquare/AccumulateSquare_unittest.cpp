//AccumulateSquare kernel test

//Asm function prototype:
//	void AccumulateSquare_asm(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, u32 height);

//Asm test function prototype:
//    	void AccumulateSquare_asm_test(unsigned char** srcAddr, unsigned char** maskAddr, float** destAddr, unsigned int width, unsigned int height);

//C function prototype:
//  	void AccumulateSquare(u8** srcAddr, u8** maskAddr, fp32** destAddr, u32 width, u32 height);

//srcAddr	 - array of pointers to input lines      
//maskAddr  - array of pointers to input lines of mask      
//destAddr	 - array of pointers for output lines    
//width  	 - width of input line
//height 	 - height of input 

#include "gtest/gtest.h"
#include "video.h"
#include "AccumulateSquare_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;

class AccumulateSquareTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char **inLines;
	float **outLinesC;
	float **outLinesAsm;
	float **aux;
	unsigned char **inMask;
	unsigned int width;
	unsigned int height;	
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

//---------------------------------------------------------------------------------------------------------------------------------------------------
//								CTest
//---------------------------------------------------------------------------------------------------------------------------------------------------

TEST_F(AccumulateSquareTest, TestCUniformInput0Mask0)
{
	width = 240;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], 240);
}

TEST_F(AccumulateSquareTest, TestCUniformInput0Mask1)
{
	width = 1920;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesC[0], 1920);
}

TEST_F(AccumulateSquareTest, TestCUniformInput0Mask01)
{
	width = 1920;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i++)
	{
		inMask[0][i++] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
		
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
}

TEST_F(AccumulateSquareTest, TestCUniformInput0Mask01Dest45)
{
	width = 640;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 45);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 45);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesC[0], 640);
}

TEST_F(AccumulateSquareTest, TestCUniformInput255Mask0)
{
	width = 240;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesC[0], 240);
}

TEST_F(AccumulateSquareTest, TestCUniformInput255Mask1)
{
	width = 320;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		aux[0][i] = 65025;
	}
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesC[0], 320);
}

TEST_F(AccumulateSquareTest, TestCUniformInput255Mask01)
{
	width = 16;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(65025, outLinesC[0][i]);
		i++;
		EXPECT_EQ(0, outLinesC[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestCUniformInput255Mask01Dest231)
{
	width = 640;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		outLinesC[0][i] = 231;
	}
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(65256, outLinesC[0][i]);
		i++;
		EXPECT_EQ(231, outLinesC[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestCRandomInputMask0)
{
	width = 240;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
}

TEST_F(AccumulateSquareTest, TestCRandomInputMask1)
{
	width = 320;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(inLines[0][i] * inLines[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestCRandomInputMask01)
{
	width = 160;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(inLines[0][i] * inLines[0][i], outLinesC[0][i]);
		i++;
		EXPECT_EQ(0, outLinesC[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestCRandomInputMask01Dest176)
{
	width = 1920;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 1; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		outLinesC[0][i] = 176;
	}
	
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(176, outLinesC[0][i]);
		i++;
		EXPECT_EQ(inLines[0][i] * inLines[0][i] + 176, outLinesC[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestCMinSize16Mask1)
{	
	width = 16;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(width, height);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	inLines[0][0] = 1;
	inLines[0][1] = 9;
	inLines[0][2] = 2;
	inLines[0][3] = 8;
	inLines[0][4] = 10;
	inLines[0][5] = 10;
	inLines[0][6] = 255;
	inLines[0][7] = 7;
	inLines[0][8] = 11;
	inLines[0][9] = 89;
	inLines[0][10] = 236;
	inLines[0][11] = 114;
	inLines[0][12] = 6;
	inLines[0][13] = 107;
	inLines[0][14] = 3;
	inLines[0][15] = 0;

	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	EXPECT_EQ(1, outLinesC[0][0]);
	EXPECT_EQ(81, outLinesC[0][1]);
	EXPECT_EQ(4, outLinesC[0][2]);
	EXPECT_EQ(64, outLinesC[0][3]);
	EXPECT_EQ(100, outLinesC[0][4]);
	EXPECT_EQ(100, outLinesC[0][5]);
	EXPECT_EQ(65025, outLinesC[0][6]);
	EXPECT_EQ(49, outLinesC[0][7]);
	EXPECT_EQ(121, outLinesC[0][8]);
	EXPECT_EQ(7921, outLinesC[0][9]);
	EXPECT_EQ(55696, outLinesC[0][10]);
	EXPECT_EQ(12996, outLinesC[0][11]);
	EXPECT_EQ(36, outLinesC[0][12]);
	EXPECT_EQ(11449, outLinesC[0][13]);
	EXPECT_EQ(9, outLinesC[0][14]);
	EXPECT_EQ(0, outLinesC[0][15]);	
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//								AsmTest
//---------------------------------------------------------------------------------------------------------------------------------------------------

TEST_F(AccumulateSquareTest, TestAsmUniformInput0Mask0)
{
	width = 240;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput0Mask1)
{
	width = 1920;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput0Mask1Dest216)
{
	width = 160;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		aux[0][i] = 216;
		outLinesAsm[0][i] = 216;
	}
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput0Mask01)
{
	width = 640;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput255Mask0)
{
	width = 96;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput255Mask1)
{
	width = 640;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		aux[0][i] = 65025;
	}
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput255Mask01)
{
	width = 160;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(65025, outLinesAsm[0][i]);
		i++;
		EXPECT_EQ(0, outLinesAsm[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestAsmUniformInput255Mask1Dest11)
{
	width = 640;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 255);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		aux[0][i] = 65036;
		outLinesAsm[0][i] = 11;
	}
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmRandomInputMask0)
{
	width = 240;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	aux = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestAsmRandomInputMask1)
{
	width = 320;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);	
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(inLines[0][i] * inLines[0][i], outLinesAsm[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestAsmRandomInputMask01)
{
	width = 32;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	for(int i = 0; i < width; i++)
	{
		EXPECT_EQ(inLines[0][i] * inLines[0][i], outLinesAsm[0][i]);
		i++;
		EXPECT_EQ(0, outLinesAsm[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestAsmRandomInputMask1Dest70)
{
	width = 320;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);	
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		outLinesAsm[0][i] = 70;
	}
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);

	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(inLines[0][i] * inLines[0][i] + 70, outLinesAsm[0][i]);
	}
}

TEST_F(AccumulateSquareTest, TestAsmMinSize16Mask1)
{	
	width = 16;
	height = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, height, 0);
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	inLines[0][0] = 1;
	inLines[0][1] = 9;
	inLines[0][2] = 2;
	inLines[0][3] = 8;
	inLines[0][4] = 10;
	inLines[0][5] = 10;
	inLines[0][6] = 255;
	inLines[0][7] = 7;
	inLines[0][8] = 11;
	inLines[0][9] = 89;
	inLines[0][10] = 236;
	inLines[0][11] = 114;
	inLines[0][12] = 6;
	inLines[0][13] = 107;
	inLines[0][14] = 3;
	inLines[0][15] = 0;
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
		
	EXPECT_EQ(1, outLinesAsm[0][0]);
	EXPECT_EQ(81, outLinesAsm[0][1]);
	EXPECT_EQ(4, outLinesAsm[0][2]);
	EXPECT_EQ(64, outLinesAsm[0][3]);
	EXPECT_EQ(100, outLinesAsm[0][4]);
	EXPECT_EQ(100, outLinesAsm[0][5]);
	EXPECT_EQ(65025, outLinesAsm[0][6]);
	EXPECT_EQ(49, outLinesAsm[0][7]);
	EXPECT_EQ(121, outLinesAsm[0][8]);
	EXPECT_EQ(7921, outLinesAsm[0][9]);
	EXPECT_EQ(55696, outLinesAsm[0][10]);
	EXPECT_EQ(12996, outLinesAsm[0][11]);
	EXPECT_EQ(36, outLinesAsm[0][12]);
	EXPECT_EQ(11449, outLinesAsm[0][13]);
	EXPECT_EQ(9, outLinesAsm[0][14]);
	EXPECT_EQ(0, outLinesAsm[0][15]);	
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//							Compare C and Asm outputs
//---------------------------------------------------------------------------------------------------------------------------------------------------

TEST_F(AccumulateSquareTest, TestRandomInputMask1)
{
	width = 1280;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
	
}

TEST_F(AccumulateSquareTest, TestRandomInputMask1Dest66)
{
	width = 320;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), height, 0);
	for(int i = 0; i < width; i++)
	{
		outLinesC[0][i] = 66;
		outLinesAsm[0][i] = 66;
	}
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
	
}

TEST_F(AccumulateSquareTest, TestRandomInputMask1DestRandom)
{
	width = 704;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), height);
	outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), height);
	for(int i = 0; i < width; i++)
	{
		outLinesAsm[0][i] = (float)randomGen.GenerateUInt(0, 255, 1);
		outLinesC[0][i] = outLinesAsm[0][i];
	}
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateSquareTest, TestRandomInputMask1DestRandomWidthMax)
{
	width = 1920;
	height = 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, height, 0, 255);
	outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), height);
	outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), height);
	for(int i = 0; i < width; i++)
	{
		outLinesAsm[0][i] = (float)randomGen.GenerateUInt(0, 255, 1);
		outLinesC[0][i] = outLinesAsm[0][i];
	}
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, height, 1);
	
	AccumulateSquare_asm_test(inLines, inMask, outLinesAsm, width, height);
	AccumulateSquare(inLines, inMask, outLinesC, width, height);
	RecordProperty("CyclePerPixel", AccumulateSquareCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}
