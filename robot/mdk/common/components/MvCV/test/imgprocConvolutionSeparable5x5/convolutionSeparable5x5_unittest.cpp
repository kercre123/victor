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
#include "convolutionSeparable5x5_asm_test.h"
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

class ConvolutionSeparable5x5KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half vconvAsm[5];
	half hconvAsm[5];
	float vconvC[5];
	float hconvC[5];

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

TEST_F(ConvolutionSeparable5x5KernelTest, TestUniformInputLinesAll0)
{
	width = 64;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, 5, 1.0);
	inputGen.FillLine(hconvC, 5, 1.0);
	inputGen.FillLine(vconvAsm, 5, 1.0);
	inputGen.FillLine(hconvAsm, 5, 1.0);

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable5x5KernelTest, TestUniformInputLinesAll255)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, 5, 1.0);
	inputGen.FillLine(hconvC, 5, 1.0);
	inputGen.FillLine(vconvAsm, 5, 1.0);
	inputGen.FillLine(hconvAsm, 5, 1.0);

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionSeparable5x5KernelTest, TestUniformInputLinesWidth16)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 7);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.FillLine(vconvC, 5, 1.0);
	inputGen.FillLine(hconvC, 5, 1.0);
	inputGen.FillLine(vconvAsm, 5, 1.0);
	inputGen.FillLine(hconvAsm, 5, 1.0);

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);

	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable5x5KernelTest, TestUniformInputRandomKernels)
{
	width = 120;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 24);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, 5, 0.0, 2.0);
	inputGen.FillLine(hconvC, 5, 0.0, 2.0);
	for(int i = 0; i < 5; i++)
	{
		vconvAsm[i] = vconvC[i];
		hconvAsm[i] = hconvC[i];
	}

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable5x5KernelTest, TestUniformKernelRandomInputs)
{
	width = 120;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("uniform");
	inputGen.FillLine(vconvC, 5, 1.0);
	inputGen.FillLine(hconvC, 5, 1.0);
	inputGen.FillLine(vconvAsm, 5, 1.0);
	inputGen.FillLine(hconvAsm, 5, 1.0);


	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionSeparable5x5KernelTest, TestRandomKernelsRandomInputs)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, 5, 0.0, 0.2);
	inputGen.FillLine(hconvC, 5, 0.0, 0.2);
	for(int i = 0; i < 5; i++)
	{
		vconvAsm[i] = vconvC[i];
		hconvAsm[i] = hconvC[i];
	}

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ parameterized tests ----------------------------------------


INSTANTIATE_TEST_CASE_P(RandomData, ConvolutionSeparable5x5KernelTest,
		Values(8, 24, 32, 104, 320, 640, 800, 1280, 1920);
);

TEST_P(ConvolutionSeparable5x5KernelTest, TestRandomData)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	interm = inputGen.GetEmptyLines(width + PADDING, 1);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("random");
	inputGen.FillLine(vconvC, 5, 0.0, 0.2);
	inputGen.FillLine(hconvC, 5, 0.0, 0.2);
	for(int i = 0; i < 5; i++)
	{
		vconvAsm[i] = vconvC[i];
		hconvAsm[i] = hconvC[i];
	}

	ConvolutionSeparable5x5(inLines, outLinesC, interm, vconvC, hconvC, width);
	ConvolutionSeparable5x5_asm_test(inLines, outLinesAsm, interm, vconvAsm, hconvAsm, width);
	RecordProperty("CyclePerPixel", convolutionSeparable5x5CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
