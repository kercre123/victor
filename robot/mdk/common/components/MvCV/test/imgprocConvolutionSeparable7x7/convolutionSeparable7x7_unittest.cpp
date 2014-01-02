//convolution7x7 kernel test
//Asm function prototype:
//     void Convolution7x7_asm(u8** in, u8** out, half conv[][3], u32 inWidth);
//
//Asm test function prototype:
//     void Convolution7x7_asm(unsigned char** in, unsigned char** out, half conv[][3], unsigned long inWidth);
//
//C function prototype:
//     void Convolution7x7(u8** in, u8** out, float conv[][3], u32 inWidth);
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
#include "convolutionSeparable7x7_asm_test.h"
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
#define KERNEL_SIZE 7
#define DELTA 1 //accepted tolerance between C and ASM results

class ConvolutionSeparable7x7KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half vconvAsm[KERNEL_SIZE];
	half hconvAsm[KERNEL_SIZE];
	float vconvC[KERNEL_SIZE];
	float hconvC[KERNEL_SIZE];

	unsigned char **interm;
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

TEST_F(ConvolutionSeparable7x7KernelTest, TestUniformInputLinesAll0)
{
	width = 64;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 0);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(vconvAsm, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvAsm, KERNEL_SIZE, 1.0);

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable7x7KernelTest, TestUniformInputLinesAll255)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(vconvAsm, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvAsm, KERNEL_SIZE, 1.0);

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable7x7KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 7);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(vconvAsm, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvAsm, KERNEL_SIZE, 1.0);

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable7x7KernelTest, TestUniformInputRandomKernels)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 24);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, KERNEL_SIZE, 0.0, 0.2);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 0.0, 0.2);
	for(int i = 0; i < KERNEL_SIZE; i++)
	{
		vconvAsm[i] = (half)vconvC[i];
		hconvAsm[i] = (half)hconvC[i];
	}

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable7x7KernelTest, TestUniformKernelRandomInputs)
{
	width = 120;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("uniform");
	inputGen.FillLine(vconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 1.0);
	inputGen.FillLine(vconvAsm, KERNEL_SIZE, 1.0);
	inputGen.FillLine(hconvAsm, KERNEL_SIZE, 1.0);


	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable7x7KernelTest, TestRandomKernelsRandomInputs)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, KERNEL_SIZE, 0.0, 0.2);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 0.0, 0.2);
	for(int i = 0; i < KERNEL_SIZE; i++)
	{
		vconvAsm[i] = vconvC[i];
		hconvAsm[i] = hconvC[i];
	}

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(RandomData, ConvolutionSeparable7x7KernelTest,
		Values(8, 24, 32, 104, 320, 640, 800, 1280, 1920);
);

TEST_P(ConvolutionSeparable7x7KernelTest, TestRandomData)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, KERNEL_SIZE, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, KERNEL_SIZE, 0.0, 0.2);
	inputGen.FillLine(hconvC, KERNEL_SIZE, 0.0, 0.2);
	for(int i = 0; i < KERNEL_SIZE; i++)
	{
		vconvAsm[i] = vconvC[i];
		hconvAsm[i] = hconvC[i];
	}

	ConvolutionSeparable7x7(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable7x7_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable7x7CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
