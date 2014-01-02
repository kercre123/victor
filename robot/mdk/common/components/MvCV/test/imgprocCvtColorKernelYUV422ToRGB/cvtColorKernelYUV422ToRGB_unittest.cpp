//cvtColorKernelYUV422ToRGB kernel test

//Asm function prototype:
//	void cvtColorKernelYUV422ToRGB_asm(u8** input, u8** rOut, u8** gOut, u8** bOut, u32 width);

//Asm test function prototype:
//    	void cvtColorKernelYUV422RGB_asm_test(unsigned char width, unsigned char **outLine, unsigned char **inLine);

//C function prototype:
//  	void cvtColorKernelYUV422ToRGB(u8** input, u8** rOut, u8** gOut, u8** bOut, u32 width);

//width	- width of input line
//input	- the input YUV422 line
//rOut	- out R plane values
//gOut	- out G plane values
//bOut	- out B plane values

#include "gtest/gtest.h"
#include "imgproc.h"
#include "cvtColorKernelYUV422ToRGB_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>


using namespace mvcv;

class cvtColorKernelYUV422ToRGBTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char** inLine;
	unsigned char** outLineC_R;
	unsigned char** outLineC_G;
	unsigned char** outLineC_B;
	unsigned char** outLineAsm_R;
	unsigned char** outLineAsm_G;
	unsigned char** outLineAsm_B;
	unsigned int width;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(cvtColorKernelYUV422ToRGBTest, TestUniformInput0)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLine = inputGen.GetLines(2 * width, 1, 0);
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.ExpectAllEQ(outLineC_R[0], width, (unsigned char)0);
	outputCheck.ExpectAllEQ(outLineC_G[0], width, (unsigned char)135);
	outputCheck.ExpectAllEQ(outLineC_B[0], width, (unsigned char)0);
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestUniformInput255)
{	
	width = 720;
	inputGen.SelectGenerator("uniform");
	inLine = inputGen.GetLines(2 * width, 1, 255);	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.ExpectAllEQ(outLineC_R[0], width, (unsigned char)255);
	outputCheck.ExpectAllInRange(outLineC_G[0], width, (unsigned char)120, (unsigned char)121);
	outputCheck.ExpectAllEQ(outLineC_B[0], width, (unsigned char)255);
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestUniformInput128)
{	
	width = 720;
	inputGen.SelectGenerator("uniform");
	inLine = inputGen.GetLines(2 * width, 1, 128);	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.ExpectAllInRange(outLineC_R[0], width, (unsigned char)127, (unsigned char)129);
	outputCheck.ExpectAllInRange(outLineC_G[0], width, (unsigned char)127, (unsigned char)129);
	outputCheck.ExpectAllInRange(outLineC_B[0], width, (unsigned char)127, (unsigned char)129);
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestRandomInput)
{	
	width = 240;
	inputGen.SelectGenerator("random");
	inLine = inputGen.GetLines(2 * width, 1, 0, 255);	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestRandomInputMinWidth)
{	
	width = 8;
	inputGen.SelectGenerator("random");
	inLine = inputGen.GetLines(2 * width, 1, 0, 255);	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestRandomInputMaxWidth)
{	
	width = 1920;
	inputGen.SelectGenerator("random");
	inLine = inputGen.GetLines(2 * width, 1, 0, 255);	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestBlackWidthMultipleOf8)
{	
	width = 88;
	inputGen.SelectGenerator("uniform");
	inLine = inputGen.GetLines(2 * width, 1, 0);	
	for(int i = 1; i < width * 2; i+=2)
		inLine[0][i] = 128;
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.ExpectAllEQ(outLineC_R[0], width, (unsigned char)0);
	outputCheck.ExpectAllEQ(outLineC_G[0], width, (unsigned char)0);
	outputCheck.ExpectAllEQ(outLineC_B[0], width, (unsigned char)0);
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}

TEST_F(cvtColorKernelYUV422ToRGBTest, TestWhiteWidthRandom)
{	
	width = randomGen.GenerateUChar(8, 255, 8);
	inputGen.SelectGenerator("uniform");
	inLine = inputGen.GetLines(2 * width, 1, 255);	
	for(int i = 1; i < width * 2; i+=2)
		inLine[0][i] = 128;
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);	
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);

	cvtColorKernelYUV422ToRGB(inLine, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorKernelYUV422ToRGB_asm_test(inLine, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorKernelYUV422ToRGBCycleCount / width);
	
	outputCheck.ExpectAllEQ(outLineC_R[0], width, (unsigned char)255);
	outputCheck.ExpectAllEQ(outLineC_G[0], width, (unsigned char)255);
	outputCheck.ExpectAllEQ(outLineC_B[0], width, (unsigned char)255);
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, 1);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, 1);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, 1);
}