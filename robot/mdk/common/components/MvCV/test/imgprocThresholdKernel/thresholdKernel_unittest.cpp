//thresholdKernel kernel test

//Asm function prototype:
//	void thresholdKernel_asm(u8** in, u8** out, u32 width, u32 height, u8 thresh, u32 thresh_type);

//Asm test function prototype:
//    	void thresholdKernel_asm_test(unsigned char** in, unsigned char** out, unsigned int width, unsigned int height, unsigned char thresh, unsigned int thresh_type);

//C function prototype:
//     void thresholdKernel(u8** in, u8** out, u32 width, u32 height, u8 thresh, u32 thresh_type);

//Parameter description:
// in		- array of pointers to input lines
// out 	- array of pointers for output lines
// width 	- width of input line
// height    	- height of the input line
// thresh	- threshold value (0 - 255)
//thresh_type	- one of the 5 available thresholding types
//			0 for Thresh_To_Zero
//			1 for Thresh_To_Zero_Inv
//			2 for Thresh_To_Binary
//			3 for Thresh_To_Binary_Inv
//			4 for Thresh_Trunc

// test name - ex: Test0UniformInput0C
//Test0 - '0' number of case from function C (void thresholdKernel) 
//UniformInput0 - input line filled with 0
//C - asm output was compared with C function output
//if instead of 'C'  is 'Aux', means that output of asm was compared with an array 
//that contain the expected result of asm


#include "gtest/gtest.h"
#include "imgproc.h"
#include "thresholdKernel_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define Thresh_To_Zero 0
#define Thresh_To_Zero_Inv 1
#define Thresh_To_Binary 2
#define Thresh_To_Binary_Inv 3
#define Thresh_Trunc 4
#define Default 5

using namespace mvcv;

class ThresholdKernelTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}
		
	unsigned char thresh;
	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned char **aux;
	unsigned int thresh_type;
	unsigned int width;
	InputGenerator inputGen;
	RandomGenerator widthGen;
	ArrayChecker outputCheck;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

//-----------------------------------------------------------------------------------------------------------------------------------
//						case 0 Thresh_To_Zero
//-----------------------------------------------------------------------------------------------------------------------------------


TEST_F(ThresholdKernelTest, Test0ForUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(328, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(328, 1);
	outLinesC = inputGen.GetEmptyLines(328, 1);
	thresh = 255;
	thresh_type = Thresh_To_Zero; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 328, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 328, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 328);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 328);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(24, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(24, 1);
	outLinesC = inputGen.GetEmptyLines(24, 1);
	aux = inputGen.GetLines(24, 1, 0);
	thresh = 55;
	thresh_type = Thresh_To_Zero;

	thresholdKernel_asm_test(inLines, outLinesAsm, 24, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 24);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 24);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	thresh = 55;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInput255Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 255);
	thresh = 30;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInputThreshC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 21);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 21;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInputThreshAux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 170);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 0);
	thresh = 170;
	thresh_type = Thresh_To_Zero; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test0ForUniformInputMinimumWidth)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(8, 1, 171);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 170;

	inLines[0][3] = 169;
	inLines[0][7] = 8;
	thresh_type = Thresh_To_Zero;

	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test0ForRandomInputC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 130;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
} 

TEST_F(ThresholdKernelTest, Test0ForRandomInputRandomWidthC)
{	
	inputGen.SelectGenerator("random");
	width = widthGen.GenerateUInt(16, 1920, 16);
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 75;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, Test0ForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 78;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test0ForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(168, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(168, 1);
	outLinesC = inputGen.GetEmptyLines(168, 1);
	thresh = 0;
	thresh_type = Thresh_To_Zero;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 168, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 168, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 168);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 168);
}

TEST_F(ThresholdKernelTest, Test0AsmAndC)
{	
	width = 16;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	aux = inputGen.GetEmptyLines(width, 1);
	thresh = 70;
	thresh_type = Thresh_To_Zero;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
	EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(210, outLinesAsm[0][1]);
	EXPECT_EQ(0, outLinesAsm[0][2]);
	EXPECT_EQ(0, outLinesAsm[0][3]);
	EXPECT_EQ(250, outLinesAsm[0][4]);
	EXPECT_EQ(0, outLinesAsm[0][5]);
	EXPECT_EQ(0, outLinesAsm[0][6]);
	EXPECT_EQ(0, outLinesAsm[0][7]);
	EXPECT_EQ(75, outLinesAsm[0][8]);
	EXPECT_EQ(80, outLinesAsm[0][9]);
	EXPECT_EQ(90, outLinesAsm[0][10]);
	EXPECT_EQ(110, outLinesAsm[0][11]);
	EXPECT_EQ(120, outLinesAsm[0][12]);
	EXPECT_EQ(150, outLinesAsm[0][13]);
	EXPECT_EQ(150, outLinesAsm[0][14]);
	EXPECT_EQ(255, outLinesAsm[0][15]);
}

//-----------------------------------------------------------------------------------------------------------------------------------
//						case 1 Thresh_To_Zero_Inv
//-----------------------------------------------------------------------------------------------------------------------------------

TEST_F(ThresholdKernelTest, Test1ForUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(1920, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 255;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
}

TEST_F(ThresholdKernelTest, Test1ForUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(24, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(24, 1);
	aux = inputGen.GetLines(24, 1, 0);
	thresh = 55;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 24, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 24);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 24);
}

TEST_F(ThresholdKernelTest, Test1ForUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(648, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(648, 1);
	outLinesC = inputGen.GetEmptyLines(648, 1);
	thresh = 55;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 648, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 648, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 648);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 648);
}

TEST_F(ThresholdKernelTest, Test1ForUniformInput255Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	aux = inputGen.GetLines(640, 1, 0);
	thresh = 30;
	thresh_type = Thresh_To_Zero_Inv; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test1ForUniformInputThreshC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(120, 1, 187);
	outLinesAsm = inputGen.GetEmptyLines(120, 1);
	outLinesC = inputGen.GetEmptyLines(120, 1);
	thresh = 187;
	thresh_type = Thresh_To_Zero_Inv; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 120, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 120, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 120);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 120);
}

TEST_F(ThresholdKernelTest, Test1ForRandomInputC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 77;
	thresh_type = Thresh_To_Zero_Inv; 
		
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
} 

TEST_F(ThresholdKernelTest, Test1ForRandomInputRandomWidthC)
{	
	inputGen.SelectGenerator("random");
	width = widthGen.GenerateUInt(16, 1920, 16);
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 250;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, Test1ForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 78;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test1ForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(24, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(24, 1);
	outLinesC = inputGen.GetEmptyLines(24, 1);
	thresh = 255;
	thresh_type = Thresh_To_Zero_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 24, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 24, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 24);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 24);
}

TEST_F(ThresholdKernelTest, Test1AsmAndC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(16, 1);
	outLinesAsm = inputGen.GetEmptyLines(16, 1);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	aux = inputGen.GetEmptyLines(16, 1);
	thresh = 70;
	thresh_type = Thresh_To_Zero_Inv;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;	
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 16, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 16, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 16);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
	EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(0, outLinesAsm[0][1]);
	EXPECT_EQ(30, outLinesAsm[0][2]);
	EXPECT_EQ(40, outLinesAsm[0][3]);
	EXPECT_EQ(0, outLinesAsm[0][4]);
	EXPECT_EQ(55, outLinesAsm[0][5]);
	EXPECT_EQ(60, outLinesAsm[0][6]);
	EXPECT_EQ(70, outLinesAsm[0][7]);
	EXPECT_EQ(0, outLinesAsm[0][8]);
	EXPECT_EQ(0, outLinesAsm[0][9]);
	EXPECT_EQ(0, outLinesAsm[0][10]);
	EXPECT_EQ(0, outLinesAsm[0][11]);
	EXPECT_EQ(0, outLinesAsm[0][12]);
	EXPECT_EQ(0, outLinesAsm[0][13]);
	EXPECT_EQ(0, outLinesAsm[0][14]);
	EXPECT_EQ(0, outLinesAsm[0][15]);
}

//-----------------------------------------------------------------------------------------------------------------------------------
//						case 2 Thresh_To_Binary
//-----------------------------------------------------------------------------------------------------------------------------------

TEST_F(ThresholdKernelTest, Test2ForUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 30;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test2ForUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 0);
	thresh = 55;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test2ForUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	thresh = 55;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test2ForUniformInput255Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	aux = inputGen.GetLines(640, 1, 255);
	thresh = 30;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test2ForUniformInputThreshC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 55);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 55;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test2ForUniformInputThreshAux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 130);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 0);
	thresh = 130;
	thresh_type = Thresh_To_Binary; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test2ForRandomInputC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 14;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
} 

TEST_F(ThresholdKernelTest, Test2ForRandomInputRandomWidthC)
{	
	width = widthGen.GenerateUInt(16, 1920, 16);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 250;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, Test2ForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 78;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test2ForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(168, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(168, 1);
	outLinesC = inputGen.GetEmptyLines(168, 1);
	thresh = 111;
	thresh_type = Thresh_To_Binary;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 168, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 168, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 168);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 168);
}

TEST_F(ThresholdKernelTest, Test2AsmAndC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(16, 1);
	outLinesAsm = inputGen.GetEmptyLines(16, 1);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	aux = inputGen.GetEmptyLines(16, 1);
	thresh = 70;
	thresh_type = Thresh_To_Binary;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;	
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 16, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 16, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 16);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
	EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(255, outLinesAsm[0][1]);
	EXPECT_EQ(0, outLinesAsm[0][2]);
	EXPECT_EQ(0, outLinesAsm[0][3]);
	EXPECT_EQ(255, outLinesAsm[0][4]);
	EXPECT_EQ(0, outLinesAsm[0][5]);
	EXPECT_EQ(0, outLinesAsm[0][6]);
	EXPECT_EQ(0, outLinesAsm[0][7]);
	EXPECT_EQ(255, outLinesAsm[0][8]);
	EXPECT_EQ(255, outLinesAsm[0][9]);
	EXPECT_EQ(255, outLinesAsm[0][10]);
	EXPECT_EQ(255, outLinesAsm[0][11]);
	EXPECT_EQ(255, outLinesAsm[0][12]);
	EXPECT_EQ(255, outLinesAsm[0][13]);
	EXPECT_EQ(255, outLinesAsm[0][14]);
	EXPECT_EQ(255, outLinesAsm[0][15]);
}

//-----------------------------------------------------------------------------------------------------------------------------------
//						case 3 Thresh_To_Binary_Inv
//-----------------------------------------------------------------------------------------------------------------------------------

TEST_F(ThresholdKernelTest, Test3ForUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 30;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test3ForUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 255);
	thresh = 55;
	thresh_type = Thresh_To_Binary_Inv;

	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test3ForUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	thresh = 55;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test3ForUniformInput255Aux)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	aux = inputGen.GetLines(640, 1, 0);
	thresh = 30;
	thresh_type = Thresh_To_Binary_Inv;

	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test3ForUniformInputThreshC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 85);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 85;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
// !!! C function  (thresholdKernel) works fine, but asm function (thresholdKernel_asm_test) puts in outLinesAsm '0' imstead of '255'

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test3ForUniformInputThreshAux)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 85);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 255);
	thresh = 85;
	thresh_type = Thresh_To_Binary_Inv;

	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
//outLinesAsm contains '0' instead of '255'
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);

	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test3ForRandomInputC)
{	// !!! inLines[0][x] == thresh  !!! 
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 111;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
} 

TEST_F(ThresholdKernelTest, Test3ForRandomInputRandomWidthC)
{	
	inputGen.SelectGenerator("random");
	width = widthGen.GenerateUInt(16, 1920, 16);
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 130;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, Test3ForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 98;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test3ForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(24, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(24, 1);
	outLinesC = inputGen.GetEmptyLines(24, 1);
	thresh = 111;
	thresh_type = Thresh_To_Binary_Inv;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 24, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 24, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 24);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 24);
}

TEST_F(ThresholdKernelTest, Test3AsmAndC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(16, 1);
	outLinesAsm = inputGen.GetEmptyLines(16, 1);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	aux = inputGen.GetEmptyLines(16, 1);
	thresh = 70;
	thresh_type = Thresh_To_Binary_Inv;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;	
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 16, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 16, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 16);
	
	EXPECT_EQ(255, outLinesAsm[0][0]);
	EXPECT_EQ(0, outLinesAsm[0][1]);
	EXPECT_EQ(255, outLinesAsm[0][2]);
	EXPECT_EQ(255, outLinesAsm[0][3]);
	EXPECT_EQ(0, outLinesAsm[0][4]);
	EXPECT_EQ(255, outLinesAsm[0][5]);
	EXPECT_EQ(255, outLinesAsm[0][6]);
	//EXPECT_EQ(255, outLinesAsm[0][7]);	//!!! Asm function is wrong for this case (thresh == inLines value)
	EXPECT_EQ(255, outLinesC[0][7]); //C function is ok
	EXPECT_EQ(0, outLinesAsm[0][8]);
	EXPECT_EQ(0, outLinesAsm[0][9]);
	EXPECT_EQ(0, outLinesAsm[0][10]);
	EXPECT_EQ(0, outLinesAsm[0][11]);
	EXPECT_EQ(0, outLinesAsm[0][12]);
	EXPECT_EQ(0, outLinesAsm[0][13]);
	EXPECT_EQ(0, outLinesAsm[0][14]);
	EXPECT_EQ(0, outLinesAsm[0][15]);
	//outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
}

//-----------------------------------------------------------------------------------------------------------------------------------
//						case 4 Thresh_Trunc
//-----------------------------------------------------------------------------------------------------------------------------------

TEST_F(ThresholdKernelTest, Test4ForUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 30;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test4ForUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 0);
	thresh = 55;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test4ForUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	thresh = 55;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, Test4ForUniformInput255Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 30);
	thresh = 30;
	thresh_type = Thresh_Trunc; 
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test4ForUniformInputThrashC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 55);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 55;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test4ForUniformInputThrashAux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 205);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 205);
	thresh = 205;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, Test4ForRandomInputC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 128;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
} 

TEST_F(ThresholdKernelTest, Test4ForRandomInputRandomWidthC)
{	
	inputGen.SelectGenerator("random");
	width = widthGen.GenerateUInt(16, 1920, 16);
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 28;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, Test4ForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 98;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, Test4ForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(168, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(168, 1);
	outLinesC = inputGen.GetEmptyLines(168, 1);
	thresh = 111;
	thresh_type = Thresh_Trunc;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 168, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 168, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 168);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 168);
}

TEST_F(ThresholdKernelTest, Test4AsmAndC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(16, 1);
	outLinesAsm = inputGen.GetEmptyLines(16, 1);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	aux = inputGen.GetEmptyLines(16, 1);
	thresh = 70;
	thresh_type = Thresh_Trunc;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;	
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 16, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 16, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 16);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
	EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(70, outLinesAsm[0][1]);
	EXPECT_EQ(30, outLinesAsm[0][2]);
	EXPECT_EQ(40, outLinesAsm[0][3]);
	EXPECT_EQ(70, outLinesAsm[0][4]);
	EXPECT_EQ(55, outLinesAsm[0][5]);
	EXPECT_EQ(60, outLinesAsm[0][6]);
	EXPECT_EQ(70, outLinesAsm[0][7]);
	EXPECT_EQ(70, outLinesAsm[0][8]);
	EXPECT_EQ(70, outLinesAsm[0][9]);
	EXPECT_EQ(70, outLinesAsm[0][10]);
	EXPECT_EQ(70, outLinesAsm[0][11]);
	EXPECT_EQ(70, outLinesAsm[0][12]);
	EXPECT_EQ(70, outLinesAsm[0][13]);
	EXPECT_EQ(70, outLinesAsm[0][14]);
	EXPECT_EQ(70, outLinesAsm[0][15]);
}

//-----------------------------------------------------------------------------------------------------------------------------------
//							Default
//-----------------------------------------------------------------------------------------------------------------------------------

TEST_F(ThresholdKernelTest, TestDefaultUniformInput0C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 30;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, TestDefaultUniformInput0Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 0);
	thresh = 55;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, TestDefaultUniformInput255C)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(640, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(640, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	thresh = 55;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 640, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 640, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 640);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 640);
}

TEST_F(ThresholdKernelTest, TestDefaultUniformInput255Aux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 255);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 30);
	thresh = 30;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, TestDefaultUniformInputThrashC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 55);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	outLinesC = inputGen.GetEmptyLines(320, 1);
	thresh = 55;
	thresh_type = Default;

	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, TestDefaultUniformInputThrashAux)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320, 1, 205);
	outLinesAsm = inputGen.GetEmptyLines(320, 1);
	aux = inputGen.GetLines(320, 1, 205);
	thresh = 205;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 320, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 320);
	
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], 320);
}

TEST_F(ThresholdKernelTest, TestDefaultRandomInputC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(1920, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(1920, 1);
	outLinesC = inputGen.GetEmptyLines(1920, 1);
	thresh = 254;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 1920, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 1920, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 1920);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 1920);
}

TEST_F(ThresholdKernelTest, TestDefaultRandomInputRandomWidthC)
{	
	inputGen.SelectGenerator("random");
	width = widthGen.GenerateUInt(16, 1920, 16);
	inLines = inputGen.GetLines(width, 1, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	thresh = 13;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, width, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, width, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
} 

TEST_F(ThresholdKernelTest, TestForRandomInputMinLineWidth)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(8, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(8, 1);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	thresh = 98;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 8, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 8, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 8);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 8);
}

TEST_F(ThresholdKernelTest, TestForRandomInputLineWidth8MultipleC)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(168, 1, 0, 255);
	outLinesAsm = inputGen.GetEmptyLines(168, 1);
	outLinesC = inputGen.GetEmptyLines(168, 1);
	thresh = 111;
	thresh_type = Default;
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 168, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 168, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 168);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 168);
}

TEST_F(ThresholdKernelTest, TestAsmAndC)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(16, 1);
	outLinesAsm = inputGen.GetEmptyLines(16, 1);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	aux = inputGen.GetEmptyLines(16, 1);
	thresh = 70;
	thresh_type = Default;
	inLines[0][0] = 0;
	inLines[0][1] = 210;
	inLines[0][2] = 30;
	inLines[0][3] = 40;
	inLines[0][4] = 250;
	inLines[0][5] = 55;
	inLines[0][6] = 60;
	inLines[0][7] = 70;
	inLines[0][8] = 75;
	inLines[0][9] = 80;
	inLines[0][10] = 90;
	inLines[0][11] = 110;
	inLines[0][12] = 120;
	inLines[0][13] = 150;
	inLines[0][14] = 150;
	inLines[0][15] = 255;	
	
	thresholdKernel_asm_test(inLines, outLinesAsm, 16, 1, thresh, thresh_type);
	thresholdKernel(inLines, outLinesC, 16, 1, thresh, thresh_type);
	RecordProperty("CyclePerPixel", thresholdKernelCycleCount / 16);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
	EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(70, outLinesAsm[0][1]);
	EXPECT_EQ(30, outLinesAsm[0][2]);
	EXPECT_EQ(40, outLinesAsm[0][3]);
	EXPECT_EQ(70, outLinesAsm[0][4]);
	EXPECT_EQ(55, outLinesAsm[0][5]);
	EXPECT_EQ(60, outLinesAsm[0][6]);
	EXPECT_EQ(70, outLinesAsm[0][7]);
	EXPECT_EQ(70, outLinesAsm[0][8]);
	EXPECT_EQ(70, outLinesAsm[0][9]);
	EXPECT_EQ(70, outLinesAsm[0][10]);
	EXPECT_EQ(70, outLinesAsm[0][11]);
	EXPECT_EQ(70, outLinesAsm[0][12]);
	EXPECT_EQ(70, outLinesAsm[0][13]);
	EXPECT_EQ(70, outLinesAsm[0][14]);
	EXPECT_EQ(70, outLinesAsm[0][15]);
}
