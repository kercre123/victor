//sobelFilter kernel test
//Asm function prototype:
//     void sobel_asm(u8** in, u8** out, u32 width);
//
//Asm test function prototype:
//     void sobel_asm_test(unsigned char** in, unsigned char** out, unsigned int width);
//
//C function prototype:
//     void sobel(u8** in, u8** out, u32 width)
//
//Parameter description:
// sobel filter - Filter, calculates magnitude
// in     - array of pointers to input lines
// out    - array of pointers for output lines
// width  - width of input line

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "sobel_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

#define DELTA 1
#define PADDING 2

class SobelFilterKernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char **inLinesC;
	unsigned char **inLinesAsm;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned int width;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};


TEST_F(SobelFilterKernelTest, TestUniformInputLines)
{
	width = 320;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 1);
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(SobelFilterKernelTest, TestUniformInputLinesAll0)
{
	width = 640;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 0);
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(SobelFilterKernelTest, TestUniformInputLinesAll255)
{
	width = 800;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 255);
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(SobelFilterKernelTest, TestDifferentUniformInputLines)
{
	width = 1280;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 1);

	for(int i = 0; i < 3; i++) {
		inputGen.FillLine(inLinesAsm[i], width, i + 1);
	}
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(SobelFilterKernelTest, TestMinimumLineWidth)
{
	width = 8;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 1);
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(SobelFilterKernelTest, TestSimpleValues)
{
	width = 8;

	inputGen.SelectGenerator("uniform");
	inLinesAsm = inputGen.GetLines(width + 2, 3, 1);

	inLinesAsm[0][0] = 0; inLinesAsm[1][0] = 0; inLinesAsm[2][0] = 0;
	inLinesAsm[0][1] = 7; inLinesAsm[0][2] = 3; inLinesAsm[0][3] = 2;
	inLinesAsm[1][1] = 4; inLinesAsm[1][2] = 5; inLinesAsm[1][3] = 1;
	inLinesAsm[2][1] = 9; inLinesAsm[2][2] = 3; inLinesAsm[2][3] = 5;
	inLinesAsm[0][9] = 0; inLinesAsm[1][9] = 0; inLinesAsm[2][9] = 0;

	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);

	EXPECT_EQ(15, outLinesAsm[0][1]);
	EXPECT_EQ(13, outLinesAsm[0][2]);
	EXPECT_EQ( 5, outLinesAsm[0][3]);
	EXPECT_EQ( 0, outLinesAsm[0][5]);

	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


//------------------- parameterized tests ------------------------
INSTANTIATE_TEST_CASE_P(RandomInputs, SobelFilterKernelTest,
		Values(8, 16, 40, 320, 648, 1024, 1280, 1920);
);

TEST_P(SobelFilterKernelTest, TestRandomInput)
{
	width = GetParam();

	inputGen.SelectGenerator("random");
	inLinesAsm = inputGen.GetLines(width + PADDING, 3, 0, 255);
	inLinesC = inputGen.GetOffsettedLines(inLinesAsm, 3, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	sobel(inLinesC, outLinesC, width);
	sobel_asm_test(inLinesAsm, outLinesAsm, width);
	RecordProperty("CyclePerPixel", SobelCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

