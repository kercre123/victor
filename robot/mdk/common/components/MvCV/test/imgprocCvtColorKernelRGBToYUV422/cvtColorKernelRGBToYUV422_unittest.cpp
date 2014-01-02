//cvtColorKernelRGBToYUV422 kernel test

//Asm function prototype:
//	cvtColorKernelRGBToYUV422_asm(u8** rIn, u8** gIn, u8** bIn, u8** output, u32 width);

//Asm test function prototype:
//    	void cvtColorKernelRGBToYUV422_asm_test(unsigned char **inputR, unsigned char **inputG, unsigned char **inputB, unsigned char **output, unsigned int width);

//C function prototype:
//  	void cvtColorKernelRGBToYUV422(u8** rIn, u8** gIn, u8** bIn, u8** output, u32 width);

//width	- width of input line
//rIn		- input R plane
//gIn		- input G plane
//bIn		- input B plane

#include "gtest/gtest.h"
#include "imgproc.h"
#include "cvtColorKernelRGBToYUV422_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>


using namespace mvcv;

class cvtColorKernelRGBToYUV422Test : public ::testing::Test {
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


TEST_F(cvtColorKernelRGBToYUV422Test, TestUniformInput0)
{
	width = 512;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 0);
	inLineG = inputGen.GetLines(width, 1, 0);
	inLineB = inputGen.GetLines(width, 1, 0);
	outLineExpected = inputGen.GetLines(2 * width, 1, 0);
	for(int i = 1; i < width * 2; i+=2)
		outLineExpected[0][i] = 128;
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);	

	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], 2 * width, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestUniformInput255)
{
	width = 720;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 255);
	inLineG = inputGen.GetLines(width, 1, 255);
	inLineB = inputGen.GetLines(width, 1, 255);
	outLineExpected = inputGen.GetLines(2 * width, 1, 255);
	for(int i = 1; i < width * 2; i+=2)
		outLineExpected[0][i] = 128;
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);	

	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], 2 * width, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestUniformInput128)
{
	width = 240;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 128);
	inLineG = inputGen.GetLines(width, 1, 128);
	inLineB = inputGen.GetLines(width, 1, 128);
	outLineExpected = inputGen.GetLines(2 * width, 1, 128);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);	

	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);
	
	outputCheck.CompareArrays(outLineC[0], outLineExpected[0], 2 * width, 1);
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputMinWidth)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputMaxWidth)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputNullR)
{
	width = 240;
	inputGen.SelectGenerator("uniform");
	inLineR = inputGen.GetLines(width, 1, 0);
	inputGen.SelectGenerator("random");
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputNullG)
{
	width = 240;
	inputGen.SelectGenerator("uniform");
	inLineG = inputGen.GetLines(width, 1, 0);
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputNullB)
{
	width = 240;
	inputGen.SelectGenerator("uniform");
	inLineB = inputGen.GetLines(width, 1, 0);
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}

TEST_F(cvtColorKernelRGBToYUV422Test, TestRandomInputWidthMultipleOf8)
{
	width = randomGen.GenerateUChar(8, 1920, 8);
	inputGen.SelectGenerator("random");
	inLineR = inputGen.GetLines(width, 1, 0, 255);
	inLineG = inputGen.GetLines(width, 1, 0, 255);
	inLineB = inputGen.GetLines(width, 1, 0, 255);
	outLineC = inputGen.GetEmptyLines(2 * width, 1);
	outLineAsm = inputGen.GetEmptyLines(2 * width, 1);		
	
	cvtColorKernelRGBToYUV422(inLineR, inLineG, inLineB, outLineC, width);
	cvtColorKernelRGBToYUV422_asm_test(inLineR, inLineG, inLineB, outLineAsm, width);
	RecordProperty("CyclePerPixel", cvtColorKernelRGBToYUV422CycleCount / width);	
	
	outputCheck.CompareArrays(outLineC[0], outLineAsm[0], 2 * width, 1);
}