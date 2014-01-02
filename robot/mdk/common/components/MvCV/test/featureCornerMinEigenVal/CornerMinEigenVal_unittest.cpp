//boxFilter kernel test
//Asm function prototype:
//     void CornerMinEigenVal_asm(u8 **in_lines, u8 **out_line, u32 width);
//
//Asm test function prototype:
//     void CornerMinEigenVal_asm_test(u8 **in_lines, u8 **out_line, u32 width);
//
//C function prototype:
//     void CornerMinEigenVal(u8 **in_lines, u8 **out_line, u32 width);
//
//Parameter description:
// CornerMinEigenVal filter - is 5x5 kernel size
// input_lines   - pointer to input pixel
// output_line   - position on line   
// width         - width of line
//
// each line needs a padding o kernel_size - 1 values at the end; the value of the padding is equal
// with the last value of line without padding

#include "gtest/gtest.h"
#include "core.h"
#include "feature.h"
#include "CornerMinEigenVal_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

#define PADDING 4
#define DELTA 1

class CornerMinEigenValKernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char normalize;
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




TEST_F(CornerMinEigenValKernelTest, TestUniformInputLines32)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	inputGen.FillLine(inLines[0], width, 25);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(CornerMinEigenValKernelTest, TestAllValuesZero)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(CornerMinEigenValKernelTest, TestAllValues255)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 255);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(CornerMinEigenValKernelTest, TestMinimumWidthSize)
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	inputGen.FillLine(inLines[0], width + PADDING, 25);
	outLinesC = inputGen.GetEmptyLines(width, 5);
	outLinesAsm = inputGen.GetEmptyLines(width, 5);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(CornerMinEigenValKernelTest, TestSimpleNotEqualInputLines)
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 1);
	outLinesC = inputGen.GetEmptyLines(width, 5);
	outLinesAsm = inputGen.GetEmptyLines(width, 5);
	for(int i = 1; i < 5; i++)
	{
		inputGen.FillLine(inLines[i], width + PADDING, i + 1);
	}

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(CornerMinEigenValKernelTest, TestConsecutiveValues)
{

	width = 40;
	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 100);
	inputGen.SelectGenerator("uniform");
	for(int i = 0; i < 5; i++) {
		for(int j = 0; j < width; j++) {
			inLines[i][j] = j;
		}
	}
	outLinesC = inputGen.GetEmptyLines(width, 5);
	outLinesAsm = inputGen.GetEmptyLines(width, 5);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays( outLinesAsm[0], outLinesC[0], width, DELTA);
}


TEST_F(CornerMinEigenValKernelTest, TestRandomWidth)
{

	width = randGen->GenerateUInt(8, 1280, 8);
	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	inputGen.FillLine(inLines[0], width + PADDING, 25);
	outLinesC = inputGen.GetEmptyLines(width, 5);
	outLinesAsm = inputGen.GetEmptyLines(width, 5);


	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(CornerMinEigenValKernelTest, TestRandomData)
{
	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width , 1);
	
	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(CornerMinEigenValKernelTest, TestRandomWidthRandomData)
{

	width = randGen->GenerateUInt(8, 1280, 8);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	inputGen.SelectGenerator("uniform");
	for(int i = 0; i < 5; i++) {
		inputGen.FillLineRange(inLines[i], width + PADDING,
				width, width + PADDING - 1, inLines[i][width - 1]);
	}
		
	outLinesC = inputGen.GetEmptyLines(width, 5);
	outLinesAsm = inputGen.GetEmptyLines(width, 5);

	CornerMinEigenVal(inLines, outLinesC, width);
	CornerMinEigenVal_asm_test(inLines, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenValCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}