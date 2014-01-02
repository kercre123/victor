//convolution15x15 kernel test
//
//Asm test function prototype:
//     void Convolution15x15_asm_test(unsigned char** in, unsigned char** out, unsigned char** interm,
//                                    half vconv[15], half vconv[15], unsigned int inWidth);
//
//C function prototype:
//     void void ConvolutionSeparable15x15(u8** in, u8** out, u8** interm,
//                                         half vconv[15], half hconv[15], u32 inWidth);
//
//Parameter description:
// in         - array of pointers to input lines
// out        - array of pointers to output lines
// interm     - array of pointers to intermediary line
// vconv      - array of values from vertical convolution
// hconv      - array of values from horizontal convolution
// inWidth    - width of input line
//

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "convolutionSeparable15x15_asm_test.h"
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


#define PADDING 16
#define KERNEL_SIZE 15
#define DELTA 1 //accepted tolerance between C and ASM results

class ConvolutionSeparable15x15KernelTest : public ::testing::TestWithParam< unsigned long > {
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

TEST_F(ConvolutionSeparable15x15KernelTest, TestUniformInputLinesAll0)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable15x15KernelTest, TestUniformInputLinesAll255)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable15x15KernelTest, TestUniformInputLinesWidth16)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable15x15KernelTest, TestUniformInputRandomKernels)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable15x15KernelTest, TestUniformKernelRandomInputs)
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


	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable15x15KernelTest, TestRandomKernelsRandomInputs)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(RandomData, ConvolutionSeparable15x15KernelTest,
		Values(8, 24, 32, 104, 320, 640, 800, 1280, 1920);
);

TEST_P(ConvolutionSeparable15x15KernelTest, TestRandomData)
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

	ConvolutionSeparable15x15(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable15x15_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable15x15CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
