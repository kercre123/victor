//convolution9x1 kernel test
//Asm function prototype:
//     void Convolution9x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
//
//Asm test function prototype:
//     void Convolution9x1_asm(unsigned char** in, unsigned char** out, half* conv, unsigned long inWidth);
//
//C function prototype:
//     void Convolution9x1(u8** in, u8** out, float* conv, u32 inWidth);
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
#include "convolution9x1_asm_test.h"
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

#define DELTA 1 //accepted tolerance between C and ASM results
#define KERNEL_SIZE 9

class Convolution9x1KernelTest : public ::testing::TestWithParam< unsigned long > {
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


TEST_F(Convolution9x1KernelTest, TestUniformInputLinesWidthAll0)
{
	width = 8;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(KERNEL_SIZE, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(KERNEL_SIZE, 1.0);

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Convolution9x1KernelTest, TestUniformInputLinesAll255)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(KERNEL_SIZE, 1.0f);
	half* convFilterAsm = inputGen.GetLineFloat16(KERNEL_SIZE, 1.0);

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Convolution9x1KernelTest, TestUniformInputDifferentFilter)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 4);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};
	half convFilterAsm[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(Convolution9x1KernelTest, TestRandomInputDifferentFilter)
{
	width = 16;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};
	half convFilterAsm[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(Convolution9x1KernelTest, TestUniformInputRandomFilter)
{
	width = 800;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 25);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	inputGen.SelectGenerator("random");
	float* convFilterC = inputGen.GetLineFloat(KERNEL_SIZE, -3.0f, 10.0f);
	half convFilterAsm[KERNEL_SIZE];
	for(int i = 0; i < KERNEL_SIZE; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution9x1KernelTest, TestRandomInputMinimumWidthSize)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float convFilterC[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};
	half convFilterAsm[KERNEL_SIZE] = {0.1, -2.5, 4.0, -6.0, 18.0, -6.0, 4.0, -2.5, 0.1};

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(Convolution9x1KernelTest, TestRandomInputRandomFilter)
{
	width = 24;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(KERNEL_SIZE, -3.0f, 10.0f);
	half convFilterAsm[KERNEL_SIZE];
	for(int i = 0; i < KERNEL_SIZE; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//-------------------- parameterized tests -------------------------------

INSTANTIATE_TEST_CASE_P(RandomInputs, Convolution9x1KernelTest,
		Values(24, 32, 328, 640, 800, 1024, 1280, 1920);
);

TEST_P(Convolution9x1KernelTest, TestRandomInputDifferentWidths)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, KERNEL_SIZE, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	float* convFilterC = inputGen.GetLineFloat(KERNEL_SIZE, -3.0f, 10.0f);
	half convFilterAsm[KERNEL_SIZE];
	for(int i = 0; i < KERNEL_SIZE; i++) {
		convFilterAsm[i] = (half)convFilterC[i];
	}

	Convolution9x1(inLines, outLinesC, convFilterC, width);
	Convolution9x1_asm_test(inLines, outLinesAsm, convFilterAsm, width);
	RecordProperty("CyclePerPixel", convolution9x1CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
