//cvtColorNV21toRGB kernel test

//Asm function prototype:
//	void cvtColorNV21toRGB_asm(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width);

//Asm test function prototype:
//    	void cvtColorNV21toRGB_asm_test(unsigned char **inputY,unsigned char **inputUV, unsigned char **rOut, unsigned char **gOut, unsigned char **bOut, unsigned int width)

//C function prototype:
//  	void cvtColorNV21toRGB(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width)

//width	- width of input line
//input	- the input YUV422 line
//rOut	- out R plane values
//gOut	- out G plane values
//bOut	- out B plane values

#include "gtest/gtest.h"
#include "imgproc.h"
#include "cvtColorNV21toRGB_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define DELTA 2


using namespace mvcv;

class cvtColorNV21toRGBTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char** inLineY;
	unsigned char** inLineUV;
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

TEST_F(cvtColorNV21toRGBTest, TestUniformInput0)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLineY = inputGen.GetLines(width, 1, 0);
	inLineUV = inputGen.GetLines(width, 1, 0);
	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA); 
}

TEST_F(cvtColorNV21toRGBTest, TestUniformInput255)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLineY = inputGen.GetLines(width, 1, 255);
	inLineUV = inputGen.GetLines(width, 1, 255);
	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestUniformInput128)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLineY = inputGen.GetLines(width, 1, 128);
	inLineUV = inputGen.GetLines(width, 1, 128);
	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestUniformInput94)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLineY = inputGen.GetLines(width, 1, 94);
	inLineUV = inputGen.GetLines(width, 1, 94);
	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestUniformInput51Width32)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLineY = inputGen.GetLines(width, 1, 51);
	inLineUV = inputGen.GetLines(width, 1, 51);
	
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestRandomInput)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inLineY = inputGen.GetLines(width, 1, 0, 255);
	inLineUV = inputGen.GetLines(width, 1, 0, 255);
	
	inputGen.SelectGenerator("uniform");
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestRandomInputWidth16)
{
	width = 16;
	inputGen.SelectGenerator("random");
	inLineY = inputGen.GetLines(width, 1, 0, 255);
	inLineUV = inputGen.GetLines(width, 1, 0, 255);
	
	inputGen.SelectGenerator("uniform");
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestRandomInputWidth88)
{
	width = 88;
	inputGen.SelectGenerator("random");
	inLineY = inputGen.GetLines(width, 1, 0, 255);
	inLineUV = inputGen.GetLines(width, 1, 0, 255);
	
	inputGen.SelectGenerator("uniform");
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}

TEST_F(cvtColorNV21toRGBTest, TestRandomInputMaxWidth)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLineY = inputGen.GetLines(width, 1, 0, 255);
	inLineUV = inputGen.GetLines(width, 1, 0, 255);
	
	inputGen.SelectGenerator("uniform");
	outLineC_R = inputGen.GetEmptyLines(width, 1);
	outLineC_G = inputGen.GetEmptyLines(width, 1);
	outLineC_B = inputGen.GetEmptyLines(width, 1);
	outLineAsm_R = inputGen.GetEmptyLines(width, 1);
	outLineAsm_G = inputGen.GetEmptyLines(width, 1);
	outLineAsm_B = inputGen.GetEmptyLines(width, 1);	

	cvtColorNV21toRGB(inLineY, inLineUV, outLineC_R, outLineC_G, outLineC_B, width);	
	cvtColorNV21toRGB_asm_test(inLineY, inLineUV, outLineAsm_R, outLineAsm_G, outLineAsm_B, width);	
	
	RecordProperty("CyclePerPixel", cvtColorNV21toRGBCycleCount / width);
	
	
	outputCheck.CompareArrays(outLineC_R[0], outLineAsm_R[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_G[0], outLineAsm_G[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_B[0], outLineAsm_B[0], width, DELTA);
}