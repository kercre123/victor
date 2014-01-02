//boxFilter kernel test
//Asm function prototype:
//     void CornerMinEigenVal_patched_asm(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
//
//Asm test function prototype:
//     void CornerMinEigenVal_patched_test(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
//
//C function prototype:
//     void CornerMinEigenVal_patched_asm(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
//
//Parameter description:
// CornerMinEigenVal_patched filter - is 5x5 kernel size
// in_lines       - pointer to input pixel
// posy           - position on line
// out_pix        - pointer to output pixel    
// width          - width of line
//
// each line needs a padding o kernel_size - 1 values at the end; the value of the padding is equal
// with the last value of line without padding

#include "gtest/gtest.h"
#include "core.h"
#include "feature.h"
#include "CornerMinEigenVal_patched_asm_test.h"
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

class CornerMinEigenVal_patchedKernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	
	unsigned char **inLines;
	unsigned char *outLinesC;
	unsigned char *outLinesAsm;
	unsigned int width;
	unsigned int posy;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};




TEST_F(CornerMinEigenVal_patchedKernelTest, TestUniformInputLines32)
{
	width   = 32;
	posy    = 2;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	inputGen.FillLine(inLines[0], width, 25);
    
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount / width);

	//EXPECT_EQ(1, 1);
	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}

TEST_F(CornerMinEigenVal_patchedKernelTest, TestAllValuesZero)
{
	width   = 32;
	posy    = 2;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 0);
	
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}


TEST_F(CornerMinEigenVal_patchedKernelTest, TestAllValues255)
{
	width   = 32;
	posy    = 2;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 255);
	
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}


TEST_F(CornerMinEigenVal_patchedKernelTest, TestMinimumWidthSize)
{
	width   = 8;
	posy    = 2;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 100);
	
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}

TEST_F(CornerMinEigenVal_patchedKernelTest, TestMaxWidthSize)
{
	width   = 1920;
	posy    = 2;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 100);
	
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}


TEST_F(CornerMinEigenVal_patchedKernelTest, TestSimpleNotEqualInputLines)
{
	width   = 32;
	posy    = 10;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 5, 100);
	
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);
	for(int i = 1; i < 5; i++)
	{
		inputGen.FillLine(inLines[i], width + PADDING, i + 1);
	}

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}


TEST_F(CornerMinEigenVal_patchedKernelTest, TestRandomWidthRandomData)
{

	width = randGen->GenerateUInt(8, 320, 2);
	posy    = 2;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 5, 0, 255);
	
	inputGen.SelectGenerator("uniform");
	for(int i = 0; i < 5; i++) {
		inputGen.FillLineRange(inLines[i], width + PADDING,
				width, width + PADDING - 1, inLines[i][width - 1]);
	}
	outLinesC = inputGen.GetEmptyLine(width);
	outLinesAsm = inputGen.GetEmptyLine(width);

	CornerMinEigenVal_patched(inLines, posy, outLinesC, width);
	CornerMinEigenVal_patched_asm_test(inLines, posy, outLinesAsm, width);
	RecordProperty("CyclePerPixel", CornerMinEigenVal_patchedCycleCount );

	outputChecker.CompareArrays(outLinesC, outLinesAsm, 1);
}


