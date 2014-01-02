//cvtColorKernelYUV422ToRGB kernel test

//Asm function prototype:
//	void cvtColorRGBtoNV21_asm(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k);

//Asm test function prototype:
//    	void cvtColorRGBtoNV21_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **Yout, unsigned char **UVout,  unsigned int width, unsigned int line);

//C function prototype:
//  	void cvtColorRGBtoNV21(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k) ;

//width	- width of input line
//input	- the input YUV422 line
//rOut	- out R plane values
//gOut	- out G plane values
//bOut	- out B plane values

#include "gtest/gtest.h"
#include "imgproc.h"
#include "cvtColorRGBtoNV21_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define DELTA 2

using namespace mvcv;


class cvtColorRGBtoNV21Test : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char** inLineR;
	unsigned char** inLineG;
	unsigned char** inLineB;
	unsigned char** outLineC_Y;
	unsigned char** outLineC_UV;
	unsigned char** outLineAsm_Y;
	unsigned char** outLineAsm_UV;

	unsigned int width;
	unsigned int line;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};


TEST_F(cvtColorRGBtoNV21Test, TestUniformInput0)
{
	width = 8;
	line = 0;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 0);
	inLineG = inputGen.GetLines(width, 1, 0);
	inLineB = inputGen.GetLines(width, 1, 0);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllEQ(outLineC_Y[0], width, (unsigned char)0);
	outputCheck.ExpectAllEQ(outLineAsm_Y[0], width, (unsigned char)0);
	
	outputCheck.ExpectAllEQ(outLineC_UV[0], width, (unsigned char)128);
	outputCheck.ExpectAllEQ(outLineAsm_UV[0], width, (unsigned char)128);
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width);
}
TEST_F(cvtColorRGBtoNV21Test, TestUniformInput0Oddline)
{
	width = 8;
	line = 1;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 0);
	inLineG = inputGen.GetLines(width, 1, 0);
	inLineB = inputGen.GetLines(width, 1, 0);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetLines(width, 1, 0);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetLines(width, 1, 0);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllEQ(outLineC_Y[0], width, (unsigned char)0);
	outputCheck.ExpectAllEQ(outLineAsm_Y[0], width, (unsigned char)0);
	
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
}
 
TEST_F(cvtColorRGBtoNV21Test, TestUniformInput255)
{
	width = 8;
	line = 0;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 255);
	inLineG = inputGen.GetLines(width, 1, 255);
	inLineB = inputGen.GetLines(width, 1, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllEQ(outLineC_Y[0], width, (unsigned char)255);
	outputCheck.ExpectAllEQ(outLineAsm_Y[0], width, (unsigned char)255);
	
	outputCheck.ExpectAllEQ(outLineC_UV[0], width, (unsigned char)128);
	outputCheck.ExpectAllEQ(outLineAsm_UV[0], width, (unsigned char)128);
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput255OddLine)
{
	width = 8;
	line = 3;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 255);
	inLineG = inputGen.GetLines(width, 1, 255);
	inLineB = inputGen.GetLines(width, 1, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllEQ(outLineC_Y[0], width, (unsigned char)255);
	outputCheck.ExpectAllEQ(outLineAsm_Y[0], width, (unsigned char)255);
	
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput128)
{
	width = 8;
	line = 0;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 128);
	inLineG = inputGen.GetLines(width, 1, 128);
	inLineB = inputGen.GetLines(width, 1, 128);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)127, (unsigned char)129);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)127, (unsigned char)129);
	
	outputCheck.ExpectAllEQ(outLineC_UV[0], width, (unsigned char)128);
	outputCheck.ExpectAllEQ(outLineAsm_UV[0], width, (unsigned char)128);
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput128OddLine)
{
	width = 8;
	line = 3;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 128);
	inLineG = inputGen.GetLines(width, 1, 128);
	inLineB = inputGen.GetLines(width, 1, 128);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)127, (unsigned char)129);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)127, (unsigned char)129);
	

	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput43)
{
	width = 8;
	line = 0;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 43);
	inLineG = inputGen.GetLines(width, 1, 43);
	inLineB = inputGen.GetLines(width, 1, 43);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)42, (unsigned char)45);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)42, (unsigned char)45);
	
	outputCheck.ExpectAllEQ(outLineC_UV[0], width, (unsigned char)128);
	outputCheck.ExpectAllEQ(outLineAsm_UV[0], width, (unsigned char)128);
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput43OddLine)
{
	width = 8;
	line = 3;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 43);
	inLineG = inputGen.GetLines(width, 1, 43);
	inLineB = inputGen.GetLines(width, 1, 43);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)42, (unsigned char)45);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)42, (unsigned char)45);
	

	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput96width32)
{
	width = 32;
	line = 0;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 96);
	inLineG = inputGen.GetLines(width, 1, 96);
	inLineB = inputGen.GetLines(width, 1, 96);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)95, (unsigned char)97);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)95, (unsigned char)97);
	
	outputCheck.ExpectAllEQ(outLineC_UV[0], width, (unsigned char)128);
	outputCheck.ExpectAllEQ(outLineAsm_UV[0], width, (unsigned char)128);
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestUniformInput96width32OddLine)
{
	width = 32;
	line = 3;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 96);
	inLineG = inputGen.GetLines(width, 1, 96);
	inLineB = inputGen.GetLines(width, 1, 96);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
	
	
	outputCheck.ExpectAllInRange(outLineC_Y[0], width, (unsigned char)95, (unsigned char)97);
	outputCheck.ExpectAllInRange(outLineAsm_Y[0], width, (unsigned char)95, (unsigned char)97);
	
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInput)
{
	width = 8;
	line = 0;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputOddLine)
{
	width = 8;
	line = 3;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputWidth16)
{
	width = 16;
	line = 0;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputWidth16OddLine)
{
	width = 16;
	line = 3;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputWidth88)
{
	width = 88;
	line = 0;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputWidth88OddLine)
{
	width = 88;
	line = 3;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputMaxWidth)
{
	width = 1920;
	line = 0;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
	outputCheck.CompareArrays(outLineC_UV[0], outLineAsm_UV[0], width, DELTA);
}

TEST_F(cvtColorRGBtoNV21Test, TestRandomInputMaxWidthOddLine)
{
	width = 1920;
	line = 3;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	
	outLineC_Y = inputGen.GetEmptyLines(width, 1);
	outLineC_UV = inputGen.GetEmptyLines(width, 1);
	
	outLineAsm_Y = inputGen.GetEmptyLines(width, 1);
	outLineAsm_UV = inputGen.GetEmptyLines(width, 1);	

	cvtColorRGBtoNV21(inLineR, inLineG, inLineB, outLineC_Y, outLineC_UV, width, line);	
	cvtColorRGBtoNV21_asm_test(inLineR, inLineG, inLineB, outLineAsm_Y, outLineAsm_UV, width, line);	
			
	outputCheck.CompareArrays(outLineC_Y[0], outLineAsm_Y[0], width, DELTA);
}