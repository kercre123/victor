//HarrisResponse kernel test

//Asm function prototype:
//	fp32 HarrisResponse_asm(u8 *data, u32 x, u32 y, u32 step_width, fp32 k);

//Asm test function prototype:
//    	float HarrisResponse_asm_test(unsigned char* data, unsigned int x, unsigned int y, unsigned int step_width, float k);

//C function prototype:
//  	fp32 HarrisResponse(u8 *data, u32 x, u32 y, u32 step_width, fp32 k);

// param[in] data			Input patch including borders
// param[in] x			X coordinate inside the patch. Only a value of 3 supported
// param[in] y			Y coordinate inside the patch. Only a value of 3 supported
// param[in] step_width		Step of the patch. Only a value 8 supported (2xradius + 2xborder)
// param[in] k			Constant that changes the response to the edges. Typically 0.02 is used
// return				Corner response value

#include "gtest/gtest.h"
#include "harris_score.h"
#include "HarrisResponse_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>
#include <math.h>

#define DELTA_PERCENT (0.001f)    // Allow 1% difference between reference and actual value

using namespace mvcv;

class HarrisResponseTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
		width = 8; // Needs an 8x8 patch to account for borders 
		height = 8; //                    ( this is specified in .asm file)
		x = 3;	//Only a value of 3 supported (see harris_score.h)
		y = 3;	//Only a value of 3 supported (see harris_score.h)
		k = 0.02;	//Typically 0.02 is used (see harris_score.h)	
	}	
	
	unsigned char* inLines;
	unsigned int width;
	unsigned int height;
	unsigned int x;
	unsigned int y;
	float k;
	float outputC;
	float outputAsm;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(HarrisResponseTest, TestUniformInput0)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(width * height, 0);

	outputAsm = HarrisResponse_asm_test(inLines, x, y, width, k);
	outputC = HarrisResponse(inLines, x, y, width, k);

	EXPECT_NEAR(outputC, outputAsm, 0.2);
}

TEST_F(HarrisResponseTest, TestUniformInput255)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(width * height, 0);

	for(int i = 1; i < height - 1; i++)
		for(int j = 1; j < width - 1; j++)
			inLines[i * width + j] = 255;	

	outputAsm = HarrisResponse_asm_test(inLines, x, y, width, k);
	outputC = HarrisResponse(inLines, x, y, width, k);

	EXPECT_NEAR(outputC, outputAsm, 0.2);
}

TEST_F(HarrisResponseTest, TestUniformInput11)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(width * height, 0);
	
	for(int i = 1; i < height - 1; i++)
		for(int j = 1; j < width - 1; j++)
			inLines[i * width + j] = 11;
	
	outputAsm = HarrisResponse_asm_test(inLines, x, y, width, k);
	outputC = HarrisResponse(inLines, x, y, width, k);

	EXPECT_NEAR(outputC, outputAsm, 0.2);
}

TEST_F(HarrisResponseTest, TestAll0BesideOneValue)
{	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(width * height, 0);
	inLines[27] = 7;
	
	outputAsm = HarrisResponse_asm_test(inLines, x, y, width, k);
	outputC = HarrisResponse(inLines, x, y, width, k);

	EXPECT_NEAR(outputC, outputAsm, 0.2);
}

TEST_F(HarrisResponseTest, TestRandomInput)
{	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLine(width * height, 0, 255, 1);
	
	outputAsm = HarrisResponse_asm_test(inLines, x, y, width, k);
	outputC = HarrisResponse(inLines, x, y, width, k);
	
    float diff = fabs(outputC - outputAsm);
    float percentDiff = (diff / outputC) * 100.0f;
    
    EXPECT_LE(percentDiff, DELTA_PERCENT);
}