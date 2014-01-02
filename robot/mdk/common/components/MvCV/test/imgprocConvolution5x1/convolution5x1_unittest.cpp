//convolution5x1 kernel test
//Asm function prototype:
//     void Convolution5x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
//
//Asm test function prototype:
//     void Convolution5x1_asm(unsigned char** in, unsigned char** out, half* conv, unsigned long inWidth);
//
//C function prototype:
//     void Convolution5x1(u8** in, u8** out, float* conv, u32 inWidth);
//
//Parameter description:
// in         - array of pointers to input lines
// out        - array of pointers to output lines
// conv       - array of values from convolution
// inWidth    - width of input line
//

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "convolution5x1_asm_test.h"
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

#define DELTA 2 //accepted tolerance between C and ASM results

class Convolution5x1KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

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


TEST_F(Convolution5x1KernelTest, TestUniformInputLinesWidthAll0)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(5, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(5, 1.0);

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Convolution5x1KernelTest, TestUniformInputLinesAll255)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(5, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(5, 1.0);

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Convolution5x1KernelTest, TestIndividualValues)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float convFilterC[5] = {0.1, -2.5, 10.0, -2.5, 0.1};
	half convFilterAsm[5] = {0.1, -2.5, 10.0, -2.5, 0.1};

	inLines[0][0] =   4;  inLines[0][1] =  23; inLines[0][2] =    0;
	inLines[1][0] =   8;  inLines[1][1] =  16; inLines[1][2] =  255;
	inLines[2][0] =  10;  inLines[2][1] = 255; inLines[2][2] =    0;
	inLines[3][0] =  12;  inLines[3][1] =   8; inLines[3][2] =  255;
	inLines[4][0] =  16;  inLines[4][1] = 190; inLines[4][2] =    0;

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	EXPECT_EQ( 51, outLinesAsm[0][0]);
	EXPECT_EQ(255, outLinesAsm[0][1]);
	EXPECT_EQ(  0, outLinesAsm[0][2]);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution5x1KernelTest, TestUniformInputDifferentFilter)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 4);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[5] = {0.1, -2.5, 10.0, -2.5, 0.1};
	half convFilterAsm[5] = {0.1, -2.5, 10.0, -2.5, 0.1};

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution5x1KernelTest, TestRandomInputDifferentFilter)
{
	width = 16;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[5] = {0.1, -2.5, 10.0, -2.5, 0.1};
	half convFilterAsm[5] = {0.1, -2.5, 10.0, -2.5, 0.1};

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution5x1KernelTest, TestUniformInputRandomFilter)
{
	width = 800;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 25);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inputGen.SelectGenerator("random");
	float* convFilterC = inputGen.GetLineFloat(5, -3.0f, 10.0f);
	half convFilterAsm[5];
	for(int i = 0; i < 5; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution5x1KernelTest, TestRandomInputMinimumWidthSize)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[5] = {0.1, -2.5, 10.0, -2.5, 0.1};
	half convFilterAsm[5] = {0.1, -2.5, 10.0, -2.5, 0.1};

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution5x1KernelTest, TestRandomInputRandomFilter)
{
	width = 24;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(5, -3.0f, 10.0f);
	half convFilterAsm[5];
	for(int i = 0; i < 5; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//-------------------- parameterized tests -------------------------------

INSTANTIATE_TEST_CASE_P(RandomInputs, Convolution5x1KernelTest,
		Values(24, 32, 328, 640, 800, 1024, 1280, 1920);
);

TEST_P(Convolution5x1KernelTest, TestRandomInputDifferentWidths)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(5, -3.0f, 10.0f);
	half convFilterAsm[5];
	for(int i = 0; i < 5; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution5x1(inLines, outLinesC, convFilterC, width);
	Convolution5x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution5x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
