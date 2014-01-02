//convolution7x7 kernel test
//Asm function prototype:
//     void Convolution7x7_asm(u8** in, u8** out, half conv[][7], u32 inWidth);
//
//Asm test function prototype:
//     void Convolution7x7_asm(unsigned char** in, unsigned char** out, half conv[][7], unsigned long inWidth);
//
//C function prototype:
//     void Convolution7x7(u8** in, u8** out, float conv[][7], u32 inWidth);
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
#include "convolution7x7_asm_test.h"
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


#define PADDING 8
#define DELTA 1 //accepted tolerance between C and ASM results

class Convolution7x7KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half convMatAsm[49];
	float convMatC[49];
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

TEST_F(Convolution7x7KernelTest, TestUniformInputLinesMinimumWidth)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 4);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convMatC, 49, 0.02);
	inputGen.FillLine(convMatAsm, 49, 0.02);

	Convolution7x7(inLines, outLinesC, convMatC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.ExpectAllInRange<unsigned char>(outLinesAsm[0], width, 3, 4);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution7x7KernelTest, TestUniformInputLinesAll0)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convMatC, 49, 0.02f);
	inputGen.FillLine(convMatAsm, 49, 0.02f);

	Convolution7x7(inLines, outLinesC, convMatC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution7x7KernelTest, TestUniformInputLinesAll255)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convMatC, 49, 0.02f);
	inputGen.FillLine(convMatAsm, 49, 0.02f);

	Convolution7x7(inLines, outLinesC, convMatC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution7x7KernelTest, TestUniformInputLinesRandomConvMatrix)
{
	width = 64;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convFilterC[49];
	half    convFilterAsm[49];
	inputGen.SelectGenerator("random");
	inputGen.FillLine(convFilterAsm, 49, 0.0f, 0.1f);
	for(int i = 0; i < 49; i++) {
		convFilterC[i] = (float)convFilterAsm[i];
	}

	Convolution7x7(inLines, outLinesC, convFilterC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution7x7KernelTest, TestRandomInputLinesRandomConvMatrix)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 7, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[49];
	half    convMatAsm[49];
	inputGen.FillLine(convMatAsm, 49, 0.0f, 0.01f);
	for(int i = 0; i < 49; i++) {
		convMatC[i] = (float)convMatAsm[i];
	}

	Convolution7x7(inLines, outLinesC, convMatC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(UniformInputs, Convolution7x7KernelTest,
		Values(16, 32, 320, 640, 800, 1280, 1920);
);

TEST_P(Convolution7x7KernelTest, TestRandomInputLinesBlurFilter)
{
	float   convMatC[49];
	half    convMatAsm[49];

	inputGen.SelectGenerator("uniform");
	inputGen.FillLine(convMatC, 49, 1.0f / 49);
	inputGen.FillLine(convMatAsm, 49, 1.0f / 49);

	unsigned int width = GetParam();

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 7, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	Convolution7x7(inLines, outLinesC, convMatC, width);
	Convolution7x7_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
