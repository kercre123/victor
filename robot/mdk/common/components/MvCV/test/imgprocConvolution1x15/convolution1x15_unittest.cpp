//convolution1x15 kernel test
//Asm function prototype:
//     void Convolution1x15_asm(u8** in, u8** out, half* conv, u32 inWidth);
//
//Asm test function prototype:
//     void Convolution1x15_asm(unsigned char** in, unsigned char** out, half* conv, unsigned long inWidth);
//
//C function prototype:
//     void Convolution1x15(u8** in, u8** out, float* conv, u32 inWidth);
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
#include "convolution1x15_asm_test.h"
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


#define PADDING 14
#define DELTA 1 //accepted tolerance between C and ASM results

class Convolution1x15KernelTest : public ::testing::TestWithParam< unsigned long > {
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


TEST_F(Convolution1x15KernelTest, TestUniformInputLinesWidthAll0)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 1, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(15, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(15, 1.0);

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution1x15KernelTest, TestUniformInputLinesAll255)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 1, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(15, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(15, 1.0);

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Convolution1x15KernelTest, TestUniformInputDifferentFilter)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 1, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	half convFilterAsm[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution1x15KernelTest, TestRandomInputDifferentFilter)
{
	width = 1200;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 1, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	half convFilterAsm[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution1x15KernelTest, TestUniformInputRandomFilter)
{
	width = 800;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 1, 20);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inputGen.SelectGenerator("random");
	float* convFilterC = inputGen.GetLineFloat(15, -3.0f, 10.0f);
	half convFilterAsm[15];
	for(int i = 0; i < 15; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution1x15KernelTest, TestRandomInputMinimumWidthSize)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 1, 20);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	half convFilterAsm[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution1x15KernelTest, TestRandomInputRandomFilter)
{
	width = 24;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 1, 20);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(15, -3.0f, 10.0f);
	half convFilterAsm[15];
	for(int i = 0; i < 15; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


//-------------------- parameterized tests -------------------------------

INSTANTIATE_TEST_CASE_P(RandomInputs, Convolution1x15KernelTest,
		Values(32, 328, 640, 800, 1024, 1280, 1920);
);

TEST_P(Convolution1x15KernelTest, TestRandomInputDifferentWidths)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 1, 20);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(15, -3.0f, 10.0f);
	half convFilterAsm[15];
	for(int i = 0; i < 15; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution1x15(inLines, outLinesC, convFilterC, width);
	Convolution1x15_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution1x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
