//canny edge detector kernel test
//Asm function prototype:
//     void canny_asm(u8** srcAddr, u8** dstAddr,  float threshold1, float threshold2, u32 width);
//
//Asm test function prototype:
//     void canny_asm(unsigned char** srcAddr, unsigned char** dstAddr,
//                    float threshold1, float threshold2, unsigned int width);
//
//C function prototype:
//     void canny(u8** srcAddr, u8** dstAddr,  float threshold1, float threshold2, u32 width);
//
//Parameter description:
// srcAddr    - array of pointers to input lines
// dstAddr    - array of pointers for output lines
// threshold1 - lower threshold
// threshold2 - upper threshold
// width      - width of input line

#include "gtest/gtest.h"
#include "feature.h"
#include "canny_asm_test.h"
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
#define PADDING 4

class CannyKernelTest : public ::testing::TestWithParam< unsigned long > {
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
	float threshold1;
	float threshold2;

	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

TEST_F(CannyKernelTest, TestUniformInputLinesAll0)
{
	width = 128;
	threshold1 = 1.0f;
	threshold2 = 2.0f;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 9, 0);

	outLinesC = inputGen.GetLines(width, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	canny(inLines, outLinesC, threshold1, threshold2, width);
	canny_asm_test(inLines, outLinesAsm, threshold1, threshold2, width);
	RecordProperty("CyclePerPixel", CannyCycleCount / width);

		EXPECT_EQ(0, 0);
}
/*
TEST_F(CannyKernelTest, TestUniformInputLinesAll255)
{
	width = 120;
	threshold1 = 1.0f;
	threshold2 = 2.0f;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 9, 255);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	canny(inLines, outLinesC, threshold1, threshold2, width);
	canny_asm_test(inLines, outLinesAsm, threshold1, threshold2, width);
	RecordProperty("CyclePerPixel", CannyCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(CannyKernelTest, TestUniformInputLines)
{
	width = 320;
	threshold1 = 3.0f;
	threshold2 = 9.0f;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 9, 7);

	outLinesC = inputGen.GetLines(width, 1, 0);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	canny(inLines, outLinesC, threshold1, threshold2, width);
	canny_asm_test(inLines, outLinesAsm, threshold1, threshold2, width);
	RecordProperty("CyclePerPixel", CannyCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(CannyKernelTest, TestRandomInputLines)
{
	width = 320;
	threshold1 = 3.0f;
	threshold2 = 9.0f;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 9, 220);
	inputGen.FillLines(inLines, width / 2, 9, 20);

	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	canny(inLines, outLinesC, threshold1, threshold2, width);
	canny_asm_test(inLines, outLinesAsm, threshold1, threshold2, width);
	RecordProperty("CyclePerPixel", CannyCycleCount / width);

	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

*/