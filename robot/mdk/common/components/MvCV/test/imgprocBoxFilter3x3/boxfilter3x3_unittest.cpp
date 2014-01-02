//boxFilter kernel test
//Asm function prototype:
//     void boxfilter3x3_asm(u8** in, u8** out, u32 normalize, u32 width);
//
//Asm test function prototype:
//     void boxfilter3x3_asm_test(u8** in, u8** out, u32 normalize, u32 width);
//
//C function prototype:
//     void boxfilter3x3(u8** in, u8** out, u32 normalize, u32 width);
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
#include "boxfilter3x3_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

#define PADDING 8
#define DELTA 1

class BoxFilter3x3KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned long normalize;
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


// ------------- normalized variant -------------------

TEST_F(BoxFilter3x3KernelTest, TestUniformInputLines32)
{
	width = 32;
	unsigned char **inLinesOffseted;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	inputGen.FillLine(inLines[0], width + PADDING * 2, 9);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);

	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width, 2);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(BoxFilter3x3KernelTest, TestAllValuesZero)
{
	width = 320;
	unsigned char **inLinesOffseted;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width, 0);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilter3x3KernelTest, TestAllValues255)
{
	width = 320;
	unsigned char **inLinesOffseted;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(BoxFilter3x3KernelTest, TestMinimumWidthSize)
{
	width = 16;
	unsigned char **inLinesOffseted;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	inputGen.FillLine(inLines[0], width + PADDING * 2, 25);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(BoxFilter3x3KernelTest, TestSimpleNotEqualInputLines)
{
	width = 320;
	unsigned char **inLinesOffseted;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 1);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	for(int i = 1; i < 3; i++)
	{
		inputGen.FillLine(inLines[i], width + PADDING * 2, i + 1);
	}

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(BoxFilter3x3KernelTest, TestRandomWidthRandomData)
{
	unsigned char **inLinesOffseted;
	width = randGen->GenerateUInt(0, 1921, 8);
	width = (width >> 3)<<3; //(line width have to be mutiple of 8)
	width = 1608;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING * 2, 3,1);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

// ----------------------- unnormalized variant ---------------------------
// the value of unnormalized output pixels can go beyond 8 bits, so
// they are stored on 16 bits values; because of this the size of the
// output buffer is twice as big than in the normalized case.




TEST_F(BoxFilter3x3KernelTest, TestUniformInputLines32Unnormalized)
{
	unsigned char **inLinesOffseted;
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 1);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], width);
}

TEST_F(BoxFilter3x3KernelTest, TestAllValuesZeroUnnormalized)
{
	unsigned char **inLinesOffseted;
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], width);
}


TEST_F(BoxFilter3x3KernelTest, TestAllValues255Unnormalized)
{
	unsigned char **inLinesOffseted;
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], width);
}


TEST_F(BoxFilter3x3KernelTest, TestMinimumWidthSizeUnnormalized)
{
	unsigned char **inLinesOffseted;
	width = 16;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], width);
}

TEST_F(BoxFilter3x3KernelTest, TestSimpleNotEqualInputLinesUnnormalized)
{
	unsigned char **inLinesOffseted;
	width = 320;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 1);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);
	for(int i = 1; i < 3; i++)
	{
		inputGen.FillLine(inLines[i], width + PADDING * 2, i + 1);
	}

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], width);
}

TEST_F(BoxFilter3x3KernelTest, TestRandomWidthRandomDataUnnormalized)
{
	unsigned char **inLinesOffseted;
	width = randGen->GenerateUInt(0, 1921);
	width = (width >> 3)<<3; //(line width have to be mutiple of 8)
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);

	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


//---------------- parameterized tests -------------------------

INSTANTIATE_TEST_CASE_P(RandomInputsUnnormalized, BoxFilter3x3KernelTest,
		Values(16, 32, 320, 640, 800, 1280, 1920);
);

TEST_P(BoxFilter3x3KernelTest, TestRandomData)
{
	unsigned char **inLinesOffseted;
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 1, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_P(BoxFilter3x3KernelTest, TestRandomDataUnnormalized)
{
	unsigned char **inLinesOffseted;
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING * 2, 3, 0, 255);
	inLinesOffseted = inputGen.GetOffsettedLines(inLines, 3, 8);
	outLinesC = inputGen.GetEmptyLines(width * 2, 1);
	outLinesAsm = inputGen.GetEmptyLines(width * 2, 1);

	boxfilter3x3(inLinesOffseted, outLinesC, 0, width);
	boxfilter3x3_asm_test(inLines, outLinesAsm, 0, width);
	RecordProperty("CyclePerPixel", boxFilter3x3CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


