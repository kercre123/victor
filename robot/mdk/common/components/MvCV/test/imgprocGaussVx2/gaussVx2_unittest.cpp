//GaussVx2 kernel test
//Asm function prototype:
//     void GaussVx2(u8 **inLine,u8 *outLine,int width);
//
//Asm test function prototype:
//     void GaussVx2_test(unsigned char** in, unsigned char* out, unsigned int width);
//
//C function prototype:
//     void GaussVx2(u8 **inLine, u8 *outLine, int width);
//
//Parameter description:
// GaussVx2 filter - downsample even lines and even cols
// inLine  - array of pointers to input lines
// outLine - pointer for output line
// width   - width of input line

#include "gtest/gtest.h"
#include "GaussVx2.h"
#include "gaussVx2_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

class GaussVx2KernelTest : public ::testing::TestWithParam< unsigned long > {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char **inLines;
	unsigned char *outLineC;
	unsigned char *outLineAsm;
	unsigned int width;

	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

TEST_F(GaussVx2KernelTest, TestUniformInputLines)
{
	width = 32;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 2);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLineAsm, width, 2);
	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}


TEST_F(GaussVx2KernelTest, TestUniformInputLinesMinimumWidthSize)
{
	width = 16;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 7);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.ExpectAllEQ<unsigned char>(outLineAsm, width, 7);
	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}


TEST_F(GaussVx2KernelTest, TestUniformInputLinesAll0)
{
	width = 328;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 0);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}


TEST_F(GaussVx2KernelTest, TestUniformInputLinesAll255)
{
	width = 1024;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 255);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}

TEST_F(GaussVx2KernelTest, TestIndividualValues)
{
	width = 64;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 5, 0);

	inLines[0][0] = 2; inLines[0][1] = 12; inLines[0][2] = 6;
	inLines[0][3] = 9; inLines[0][4] = 32; inLines[0][5] = 7;

	inLines[1][0] = 8; inLines[1][1] = 0; inLines[1][2] = 24;
	inLines[1][3] = 40; inLines[1][4] = 5; inLines[1][5] = 18;

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	EXPECT_EQ(2, outLineAsm[0]);
	EXPECT_EQ(0, outLineAsm[1]);
	EXPECT_EQ(6, outLineAsm[2]);
	EXPECT_EQ(10, outLineAsm[3]);
	EXPECT_EQ(3, outLineAsm[4]);
	EXPECT_EQ(4, outLineAsm[5]);
	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}


TEST_F(GaussVx2KernelTest, TestRandomInputLines)
{
	width = 48;

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}


//------------ parameterized tests ----------------------------------------

INSTANTIATE_TEST_CASE_P(TestDifferentInputSizes, GaussVx2KernelTest,
		Values(16, 32, 40, 320, 640, 800, 808, 1280, 1920);
);


TEST_P(GaussVx2KernelTest, TestRandom)
{
	width = GetParam();

	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 5, 0, 255);

	outLineC = inputGen.GetEmptyLine(width);
	outLineAsm = inputGen.GetEmptyLine(width);

	GaussVx2(inLines, outLineC, width);
	GaussVx2_asm_test(inLines, outLineAsm, width);
	RecordProperty("CyclePerPixel", GaussVx2CycleCount / width);

	outputChecker.CompareArrays(outLineC, outLineAsm, width);
}
