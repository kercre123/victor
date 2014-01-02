//convolution kernel test
//Asm function prototype:
//     void Convolution_asm(u8** in, u8** out, u32 kernelSize, half** conv, u32 inWidth);
//
//Asm test function prototype:
//     void Convolution_asm(unsigned char** in, unsigned char** out, u32 kernelSize, half** conv, unsigned long inWidth);
//
//C function prototype:
//     void Convolution(u8** in, u8** out, unsigned long kernelSize, float** conv, u32 inWidth);
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
#include "convolution_asm_test.h"
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

class ConvolutionKernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	half* convMatAsm;
	float* convMatC;
	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned int width;
	unsigned int kernelSize;
	unsigned int padding;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};


TEST_F(ConvolutionKernelTest, TestUniformInputLinesMinimumWidth)
{
	width = 16;
	kernelSize = 3;
	padding = 4;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 4);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	convMatC = inputGen.GetLineFloat(kernelSize * kernelSize, 1.0f);
	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 1.0f);

	Convolution(inLines, outLinesC, kernelSize, convMatC, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionKernelTest, TestUniformDifferentInputLines)
{
	width = 32;
	kernelSize = 3;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 3);
	inputGen.FillLine(inLines[1], width + padding, 6);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	convMatC = inputGen.GetLineFloat(kernelSize * kernelSize, 1.0f);
	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 1.0f);

	Convolution(inLines, outLinesC, kernelSize, convMatC, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(ConvolutionKernelTest, Test3x3ConvolutionEmbossFilter)
{
	width = 1280;
	kernelSize = 3;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 20);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	float convMat3x3C[9] = {-18.0f, -9.0f,  0.0f,
                             -9.0f,  9.0f,  9.0f,
                              0.0f,  9.0f, 18.0f };

	convMatAsm = (half*)inputGen.GetEmptyLine(kernelSize * kernelSize* sizeof(half));
	convMatAsm[0] = -18.0f; convMatAsm[1] =  -9.0f; convMatAsm[2] =  0.0f;
	convMatAsm[3] =  -9.0f; convMatAsm[4] =   9.0f; convMatAsm[5] =  9.0f;
	convMatAsm[6] =   0.0f; convMatAsm[7] =   9.0f; convMatAsm[8] = 18.0f;

	Convolution3x3(inLines, outLinesC, convMat3x3C, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionKernelTest, Test5x5ConvolutionBlurFilter)
{
	width = 1920;
	kernelSize = 5;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 20);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 0.04f);

	float convMat5x5C[25];
	inputGen.FillLine(convMat5x5C, 25, 0.04f);

	Convolution5x5(inLines, outLinesC, convMat5x5C, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(ConvolutionKernelTest, Test7x7ConvolutionBlurFilter)
{
	width = 320;
	kernelSize = 7;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 1);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 0.02f);

	float convMat7x7C[49];
	inputGen.FillLine(convMat7x7C, 49, 0.02f);

	Convolution7x7(inLines, outLinesC, convMat7x7C, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(UniformInputs, ConvolutionKernelTest,
		Values(3, 5, 7);
);


TEST_P(ConvolutionKernelTest, TestRandomLinesBlurMatrix)
{
	width = 640;
	kernelSize = GetParam();
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	inputGen.SelectGenerator("uniform");
	convMatC = inputGen.GetLineFloat(kernelSize * kernelSize, 0.02f);
	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 0.02f);

	Convolution(inLines, outLinesC, kernelSize, convMatC, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_P(ConvolutionKernelTest, TestRandomLinesRandomKernels)
{
	width = 800;
	kernelSize = GetParam();
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	convMatAsm = inputGen.GetLineFloat16(kernelSize * kernelSize, 0.0f, 0.04f);
	convMatC = inputGen.GetLineFloat(kernelSize * kernelSize, 0.0f, 0.04f);
	for(unsigned int i = 0; i < kernelSize * kernelSize; i++){
	    convMatAsm[i] = (half)convMatC[i];
	}

	Convolution(inLines, outLinesC, kernelSize, convMatC, width);
	Convolution_asm_test(inLines, outLinesAsm, kernelSize, convMatAsm, width);
	RecordProperty("CyclePerPixel", convolutionCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}
