//boxFilter kernel test
//Asm function prototype:
//     void boxfilter9x9_asm(u8** in, u8** out, u8 normalize, u32 width);
//
//Asm test function prototype:
//     void boxfilter9x9_asm_test(u8** in, u8** out, u8 normalize, u32 width);
//
//C function prototype:
//     void boxfilter9x9(u8** in, u8** out, u8 normalize, u32 width);
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
#include "boxfilter9x9_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;

#define PADDING 8
#define DELTA 1

class BoxFilter9x9KernelTest : public ::testing::Test {
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


// ------------- normalized variant -------------------

TEST_F(BoxFilter9x9KernelTest, TestUniformInputLines32)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(32 + PADDING, 9, 1);
	outLinesC = inputGen.GetEmptyLines(32, 9);
	outLinesAsm = inputGen.GetEmptyLines(32, 9);

	boxfilter9x9(inLines, outLinesC, 1, 32);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, 32);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 32);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 32);
}

TEST_F(BoxFilter9x9KernelTest, TestAllValuesZero)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320 + PADDING, 9, 0);
	outLinesC = inputGen.GetEmptyLines(320, 9);
	outLinesAsm = inputGen.GetEmptyLines(320, 9);

	boxfilter9x9(inLines, outLinesC, 1, 320);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, 320);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 320);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
	//compareOutputs(outLinesC[0], outLinesAsm[0], 320, 1);
}

TEST_F(BoxFilter9x9KernelTest, TestAllValues255)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320 + PADDING, 9, 255);
	outLinesC = inputGen.GetEmptyLines(320, 9);
	outLinesAsm = inputGen.GetEmptyLines(320, 9);

	boxfilter9x9(inLines, outLinesC, 1, 320);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, 320);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 320);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}


TEST_F(BoxFilter9x9KernelTest, TestMinimumWidthSize)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(16 + PADDING, 9, 255);
	outLinesC = inputGen.GetEmptyLines(16, 9);
	outLinesAsm = inputGen.GetEmptyLines(16, 9);

	boxfilter9x9(inLines, outLinesC, 1, 16);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, 16);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 16);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
}

TEST_F(BoxFilter9x9KernelTest, TestSimpleNotEqualInputLines)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320 + PADDING, 9, 1);
	outLinesC = inputGen.GetEmptyLines(320, 9);
	outLinesAsm = inputGen.GetEmptyLines(320, 9);
	for(int i = 1; i < 9; i++)
	{
		inputGen.FillLine(inLines[i], 320 + PADDING, i + 1);
	}

	boxfilter9x9(inLines, outLinesC, 1, 320);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, 320);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 320);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}


TEST_F(BoxFilter9x9KernelTest, TestWidth240RandomData)
{
	width = 240;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}



TEST_F(BoxFilter9x9KernelTest, TestWidth320RandomData)
{

	width = 320;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(BoxFilter9x9KernelTest, TestWidth640RandomData)
{

	width = 640;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", (float)boxFilter9x9CycleCount);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(BoxFilter9x9KernelTest, TestWidth1280RandomData)
{

	width = 1280;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

TEST_F(BoxFilter9x9KernelTest, TestWidth1920RandomData)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}


TEST_F(BoxFilter9x9KernelTest, TestWidthRandomWidthRandomData)
{

	width = randGen->GenerateUInt(0, 1921, 8);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 9, 0, 255);
	outLinesC = inputGen.GetEmptyLines(width, 9);
	outLinesAsm = inputGen.GetEmptyLines(width, 9);

	boxfilter9x9(inLines, outLinesC, 1, width);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 1, width);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

// ----------------------- unnormalized variant ---------------------------
// the value of unnormalized output pixels can go beyond 8 bits, so
// they are stored on 16 bits values; because of this the size of the
// output buffer is twice as big than in the normalized case.

TEST_F(BoxFilter9x9KernelTest, TestUniformInputLines32Unnormalized)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(32 + PADDING, 9, 1);
	outLinesC = inputGen.GetEmptyLines(32 * 2, 9);
	outLinesAsm = inputGen.GetEmptyLines(32 * 2, 9);

	boxfilter9x9(inLines, outLinesC, 0, 32);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 0, 32);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 32);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], 32);
}

TEST_F(BoxFilter9x9KernelTest, TestAllValuesZeroUnnormalized)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320 + PADDING, 9, 0);
	outLinesC = inputGen.GetEmptyLines(320 * 2, 9);
	outLinesAsm = inputGen.GetEmptyLines(320 * 2, 9);

	boxfilter9x9(inLines, outLinesC, 0, 320);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 0, 320);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 320);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], 320);
}


TEST_F(BoxFilter9x9KernelTest, TestAllValues255Unnormalized)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(320 + PADDING, 9, 255);
	outLinesC = inputGen.GetEmptyLines(320 * 2, 9);
	outLinesAsm = inputGen.GetEmptyLines(320 * 2, 9);

	boxfilter9x9(inLines, outLinesC, 0, 320);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 0, 320);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 320);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], 320);
}


TEST_F(BoxFilter9x9KernelTest, TestMinimumWidthSizeUnnormalized)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(16 + PADDING, 9, 255);
	outLinesC = inputGen.GetEmptyLines(16 * 2, 9);
	outLinesAsm = inputGen.GetEmptyLines(16 * 2, 9);

	boxfilter9x9(inLines, outLinesC, 0, 16);
	boxfilter9x9_asm_test(inLines, outLinesAsm, 0, 16);
	RecordProperty("CyclePerPixel", boxFilter9x9CycleCount / 16);

	outputChecker.CompareArrays((unsigned short*)outLinesC[0], (unsigned short*)outLinesAsm[0], 16);
}

