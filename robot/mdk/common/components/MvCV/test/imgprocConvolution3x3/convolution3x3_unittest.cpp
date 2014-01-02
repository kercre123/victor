//convolution3x3 kernel test
//Asm function prototype:
//     void Convolution3x3_asm(u8** in, u8** out, half conv[][3], u32 inWidth);
//
//Asm test function prototype:
//     void Convolution3x3_asm(unsigned char** in, unsigned char** out, half conv[][3], unsigned long inWidth);
//
//C function prototype:
//     void Convolution3x3(u8** in, u8** out, float conv[][3], u32 inWidth);
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
#include "convolution3x3_asm_test.h"
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

class Convolution3x3KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half convAsm[9];
	float convC[9];
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


TEST_F(Convolution3x3KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 3);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(convC, 9, 1.0f / 9);
	inputGen.FillLine(convAsm, 9, 1.0f / 9);

	Convolution3x3(inLines, outLinesC, convC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.ExpectAllInRange<unsigned char>(outLinesAsm[0], width, 2, 3);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution3x3KernelTest, TestUniformInputLinesAllValues0)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9] = {-18.0 / 9, -9.0 / 9, 0.0 / 9, -9.0 / 9, 9.0 / 9, 9.0 / 9, 0.0 / 9, 9.0 / 9, 18.0 / 9};
	half  convMatAsm[9] = {-18.0 / 9, -9.0 / 9, 0.0 / 9, -9.0 / 9, 9.0 / 9, 9.0 / 9, 0.0 / 9, 9.0 / 9, 18.0 / 9}; //emboss

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution3x3KernelTest, TestUniformInputLinesAllValues255)
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 3, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9] = {1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9};
	half  convMatAsm[9] = {1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9}; //emboss

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.ExpectAllInRange<unsigned char>(outLinesAsm[0], width, 254, 255);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ test different convolution matrices ------------------------

TEST_F(Convolution3x3KernelTest, TestRandomLinesSobel)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9] = {-1.0 / 9, 0.0 / 9, 1.0 / 9, -2.0 / 9, 0.0 / 9, 2.0 / 9, -1.0 / 9, 0.0 / 9, 1.0 / 9};
	half  convMatAsm[9] = {-1.0 / 9, 0.0 / 9, 1.0 / 9, -2.0 / 9, 0.0 / 9, 2.0 / 9, -1.0 / 9, 0.0 / 9, 1.0 / 9};

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution3x3KernelTest, TestRandomLinesEdgeDetect)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9] = {0.0 / 9, 1.0 / 9, 0.0 / 9, 1.0 / 9, -4.0 / 9, 1.0 / 9, 0.0 / 9, 1.0 / 9, 0.0 / 9};
	half  convMatAsm[9] = {0.0 / 9, 1.0 / 9, 0.0 / 9, 1.0 / 9, -4.0 / 9, 1.0 / 9, 0.0 / 9, 1.0 / 9, 0.0 / 9};

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution3x3KernelTest, TestRandomLinesSharpen)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9] = {0.0 / 9, -1.0 / 9, 0.0 / 9, -1.0 / 9, 5.0 / 9, -1.0 / 9, 0.0 / 9, -1.0 / 9, 0.0 / 9};
	half  convMatAsm[9] = {0.0 / 9, -1.0 / 9, 0.0 / 9, -1.0 / 9, 5.0 / 9, -1.0 / 9, 0.0 / 9, -1.0 / 9, 0.0 / 9};

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution3x3KernelTest, TestRandomLinesRandomKernel)
{
	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float   convMatC[9];
	half  convMatAsm[9];

	for(int i = 0; i < 9; i++)
	{
		convMatC[i] = randGen->GenerateFloat(-20, 20) / 9;
		convMatAsm[i] = (half)convMatC[i];

	}

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(UniformInputs, Convolution3x3KernelTest,
		Values(16, 32, 320, 640, 800, 1280, 1920);
);

TEST_P(Convolution3x3KernelTest, TestRandomInputLinesEmbossFilter)
{
	float   convMatC[9] = {-18.0 / 9, -9.0 / 9, 0.0 / 9, -9.0 / 9, 9.0 / 9, 9.0 / 9, 0.0 / 9, 9.0 / 9, 18.0 / 9};
	half  convMatAsm[9] = {-18.0 / 9, -9.0 / 9, 0.0 / 9, -9.0 / 9, 9.0 / 9, 9.0 / 9, 0.0 / 9, 9.0 / 9, 18.0 / 9};

	width = GetParam();

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 3, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	Convolution3x3(inLines, outLinesC, convMatC, width);
	Convolution3x3_asm_test(inLines, outLinesAsm, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolution3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
