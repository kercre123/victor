//boxFilter kernel test
//Asm function prototype:
//     void boxfilter_asm(u8** in, u8** out, u32 normalize, u32 width);
//
//Asm test function prototype:
//     void boxfilter_asm_test(u8** in, u8** out, u32 normalize, u32 width);
//
//C function prototype:
//     void boxfilter(u8** in, u8** out, u32 normalize, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
// width     - width of input line
//
// each line needs a padding o kernel_size - 1 values at the end; the value of the padding is equal
// with the last value of line without padding

#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "boxfilter_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

#define DELTA 0

class BoxFilterKernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned long normalize;
	unsigned int kernelSize;
	unsigned int padding;
	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned long width;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};


// ------------- normalized variant -------------------

TEST_F(BoxFilterKernelTest, TestUniform3x3InputLines)
{
	width = 32;
	kernelSize = 3;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 9);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 3);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestUniform5x5InputLines)
{
	width = 320;
	kernelSize = 5;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 25);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 5);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(BoxFilterKernelTest, TestUniform7x7InputLines)
{
	width = 320;
	kernelSize = 7;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 49);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 7);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestUniform9x9InputLines)
{
	width = 640;
	kernelSize = 9;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 81);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 9);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestUniform15x15InputLines)
{
	width = 640;
	kernelSize = 15;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0);
	inputGen.FillLine(inLines[0], width + padding, 225);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 15);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestRandomInputs3x3)
{
	width = 320;
	kernelSize = 3;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(unsigned int i = 0; i < kernelSize; i++) {
		inputGen.FillLineRange(inLines[i], width, width, width + padding - 1, inLines[i][width - 1]);
	}

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLines, outLinesC, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestRandomInputs5x5)
{
	width = 640;
	kernelSize = 5;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(unsigned int i = 0; i < kernelSize; i++) {
		inputGen.FillLineRange(inLines[i], width, width, width + padding - 1, inLines[i][width - 1]);
	}

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter5x5(inLines, outLinesC, 1, (unsigned int)width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilterKernelTest, TestRandomInputs7x7)
{
	width = 800;
	kernelSize = 7;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(unsigned int i = 0; i < kernelSize; i++) {
		inputGen.FillLineRange(inLines[i], width, width, width + padding - 1, inLines[i][width - 1]);
	}

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter7x7(inLines, outLinesC, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(BoxFilterKernelTest, TestRandomInputs9x9)
{
	width = 1280;
	kernelSize = 9;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(unsigned int i = 0; i < kernelSize; i++) {
		inputGen.FillLineRange(inLines[i], width, width, width + padding - 1, inLines[i][width - 1]);
	}

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter9x9(inLines, outLinesC, 1, (unsigned int)width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(BoxFilterKernelTest, TestRandomInputs15x15)
{
	width = 1920;
	kernelSize = 15;
	padding = kernelSize - 1;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + padding, kernelSize, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(unsigned int i = 0; i < kernelSize; i++) {
		inputGen.FillLineRange(inLines[i], width, width, width + padding - 1, inLines[i][width - 1]);
	}

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter(inLines, outLinesC, kernelSize, 1, width);
	boxfilter_asm_test(inLines, outLinesAsm, kernelSize, 1, width);
	RecordProperty("CyclePerPixel", boxFilterCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}
