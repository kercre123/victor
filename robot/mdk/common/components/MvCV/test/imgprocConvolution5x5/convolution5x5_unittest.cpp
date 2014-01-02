//convolution5x5 kernel test
//Asm function prototype:
//     void Convolution5x5_asm(u8** in, u8** out, half conv[][3], u32 inWidth);
//
//Asm test function prototype:
//     void Convolution5x5_asm(unsigned char** in, unsigned char** out, half conv[][3], unsigned long inWidth);
//
//C function prototype:
//     void Convolution5x5(u8** in, u8** out, float conv[][3], u32 inWidth);
//
//Parameter description:
// in         - array of pointers to input lines
// out        - array of pointers to output lines
// conv       - array of values from convolution
// inWidth    - width of input line
//
// each line needs a padding o kernel_size - 1 values at the end; the value of the padding is equal
// with the last value of line without padding

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "convolution5x5_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>
#include "half.h"

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;


#define PADDING 4
#define DELTA 1 //accepted tolerance between C and ASM results

class Convolution5x5KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half convMatAsm[25];
	float convMatC[25];
	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned int width;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};


TEST_F(Convolution5x5KernelTest, TestUniformInputLinesWidth16)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convMatC, 25, 0.04f);
	inputGen.FillLine(convMatAsm, 25, 0.04f);

	Convolution5x5(inLines, outLinesC, convMatC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 3);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution5x5KernelTest, TestMinimumWidth)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 7);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convMatC, 25, 1.0f / 25.0);
	inputGen.FillLine(convMatAsm, 25, 1.0f / 25.0);

	Convolution5x5(inLines, outLinesC, convMatC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 7);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution5x5KernelTest, TestSobelFilter)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float convC[25] = { 2.0 / 25.0, 1.0 / 25.0, 0.0 / 25.0, -1.0 / 25.0, -2.0 / 25.0,
			            3.0 / 25.0, 2.0 / 25.0, 0.0 / 25.0, -2.0 / 25.0, -3.0 / 25.0,
			            4.0 / 25.0, 3.0 / 25.0, 0.0 / 25.0, -3.0 / 25.0, -4.0 / 25.0,
			            3.0 / 25.0, 2.0 / 25.0, 0.0 / 25.0, -2.0 / 25.0, -3.0 / 25.0,
			    	    2.0 / 25.0, 1.0 / 25.0, 0.0 / 25.0, -1.0 / 25.0, -2.0 / 25.0 };

	for(unsigned int i = 0; i < 25; i++)
	{
		convMatAsm[i] = (half)convC[i];
	}

	Convolution5x5(inLines, outLinesC, convC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution5x5KernelTest, TestEmbossFilter)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float convC[25] = { -1.0 / 25.0, -1.0 / 25.0, -1.0 / 25.0, -1.0 / 25.0, 0.0 / 25.0,
			            -1.0 / 25.0, -1.0 / 25.0, -1.0 / 25.0,  0.0 / 25.0, 1.0 / 25.0,
			            -1.0 / 25.0, -1.0 / 25.0,  0.0 / 25.0,  1.0 / 25.0, 1.0 / 25.0,
			            -1.0 / 25.0,  0.0 / 25.0,  1.0 / 25.0,  1.0 / 25.0, 1.0 / 25.0,
			    	     0.0 / 25.0,  1.0 / 25.0,  1.0 / 25.0,  1.0 / 25.0, 1.0 / 25.0 };

	for(unsigned int i = 0; i < 25; i++)
	{
		convMatAsm[i] = (half)convC[i];
	}

	Convolution5x5(inLines, outLinesC, convC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ parameterized tests ----------------------------------------


INSTANTIATE_TEST_CASE_P(RandomInputs, Convolution5x5KernelTest,
		Values(16, 32, 320, 640, 800, 1280, 1920);
);

TEST_P(Convolution5x5KernelTest, TestRandomInputLinesBlurFilter)
{

	unsigned int width = GetParam();

	inputGen.SelectGenerator("uniform");
	inputGen.FillLine(convMatC, 25, 1.0f / 25.0);
	inputGen.FillLine(convMatAsm, 25, 1.0f / 25.0);

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	Convolution5x5(inLines, outLinesC, convMatC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_P(Convolution5x5KernelTest, TestRandomInputLinesRandomMatrix)
{
	unsigned int width = GetParam();
	inputGen.SelectGenerator("random");

	inputGen.FillLine(convMatC, 25, 0.0f, 2.0f / 25.0);

	for(unsigned int i = 0; i < 25; i++)
	{
		convMatAsm[i] = (half)convMatC[i];
	}

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	Convolution5x5(inLines, outLinesC, convMatC, width);
	Convolution5x5_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
