//minMaxKernel test
//Asm function prototype:
//    void minMaxPos_asm(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal,u8* minPos, u8* maxPos, u8* maskAddr);
//
//Asm test function prototype:
//    void minMaxPos_asm_test(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* minPos, u8* maxPos, u8* maskAddr);
//    - restrictions: works only for line size multiple of 16
//
//C function prototype:
//    void minMaxKernel(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
//
//Parameter description:
//    in - input lines
//    width - input lines width
//    height - number of input lines
//    minVal - returns minimum pixel value
//    minVal - returns maximum pixel value
//    minPox - return position of minimum pixel value
//    maxPos - return position of maximum pixel value
//    maskAddr - pixel mask: only pixels having mask value of 1 will be processed
//
//Test cases to be implemented
//    1. uniform values for the entire line
//       - expected result: both min and max have the same value with that in the input line
//    2. nominal test: minimum and maximum located in various position in the input line
//       - expected result: min and max are calculated correctly by both C and asm functions
//    3. boundary tests: check if minimum and maximum are correctly identified when located on the first or last position in the input line
//       - expected result: min and max are calculated correctly by both C and asm functions
//    4. different mask patterns:
//       - expected results: min and max are calculated correctly, considering mask configuration
//    5. different input line sizes:
//       - at least one test for each common line size: 320, 680, 800, 1280, 1920
//    6. random input values and random mask values
//        - expected results: output results are the same for both C and asm functions

#include "gtest/gtest.h"
#include "core.h"
#include "minMaxPos_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "FunctionInfo.h"
#include <ctime>
#include <memory>

using namespace mvcv;

class MinMaxPosTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());

		asmMinVal = 255;
		asmMaxVal = 0;
		asmMinPos = 0;
		asmMaxPos = 0;
		cMinVal = 255;
		cMaxVal = 0;
		cMinPos = 0;
		cMaxPos = 0;
	}

	unsigned char** inputLines;
	unsigned char* mask;
	u32 width;
	u8 asmMinVal;
	u8 asmMaxVal;
	u32 asmMinPos;
	u32 asmMaxPos;
	u8 cMinVal;
	u8 cMaxVal;
	u32 cMinPos;
	u32 cMaxPos;
	u8* line;
	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	// virtual void TearDown() {}
};

TEST_F(MinMaxPosTest, TestUniformInputLine)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 15);
	mask = inputGen.GetLine(width, 1);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(15, asmMinVal);
	EXPECT_EQ(15, asmMaxVal);
	EXPECT_EQ(0, asmMinPos);
	EXPECT_EQ(0, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAsmAllValues255)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 255);
	mask = inputGen.GetLine(width, 1);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(255, asmMinVal);
	EXPECT_EQ(255, asmMaxVal);
	EXPECT_EQ(0, asmMinPos);
	EXPECT_EQ(0, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAsmAllValuesZero) 
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 0);
	mask = inputGen.GetLine(width, 1);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(0, asmMinVal);
	EXPECT_EQ(0, asmMaxVal);
	EXPECT_EQ(0, asmMinPos);
	EXPECT_EQ(0, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAsmAllValuesMasked) 
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 7);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][301] = 127;
	inputLines[0][25] = 5;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(5, asmMinVal);
	EXPECT_EQ(127, asmMaxVal);
	EXPECT_EQ(25, asmMinPos);
	EXPECT_EQ(301, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAsmLowestLineSize) 
{
	width = 16;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 10);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][5] = 100;
	inputLines[0][15] = 2;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(2, asmMinVal);
	EXPECT_EQ(100, asmMaxVal);
	EXPECT_EQ(15, asmMinPos);
	EXPECT_EQ(5, asmMaxPos);
}


TEST_F(MinMaxPosTest, TestLefmostMinRightmostMax) 
{
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(320, 1, 5);
	mask = inputGen.GetLine(320, 1);

	inputLines[0][0] = 1;
	inputLines[0][319] = 254;

	minMaxPos_asm_test(inputLines, 320, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(1, asmMinVal);
	EXPECT_EQ(254, asmMaxVal);
	EXPECT_EQ(0, asmMinPos);
	EXPECT_EQ(319, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestLefmostMaxRightmostMin) 
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 4);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][0] = 50;
	inputLines[0][width-1] = 0;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(0, asmMinVal);
	EXPECT_EQ(50, asmMaxVal);
	EXPECT_EQ(width-1, asmMinPos);
	EXPECT_EQ(0, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestMinAndMaxValuesUnmasked_tmp)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 12);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][12] = 50;
	inputLines[0][13] = 49;
	inputLines[0][24] = 0;
	inputLines[0][25] = 1;
	mask[12] = 0;
	mask[24] = 0;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(1, asmMinVal);
	EXPECT_EQ(49, asmMaxVal);
	EXPECT_EQ(25, asmMinPos);
	EXPECT_EQ(13, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestMinAndMaxValuesUnmasked) 
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 4);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][120] = 50;
	inputLines[0][121] = 49;
	inputLines[0][240] = 0;
	inputLines[0][241] = 1;
	mask[120] = 0;
	mask[240] = 0;

	minMaxPos_asm_test(inputLines, 320, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(1, asmMinVal);
	EXPECT_EQ(49, asmMaxVal);
	EXPECT_EQ(241, asmMinPos);
	EXPECT_EQ(121, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestOnlyOneMaskedValue) 
{
	width = 320;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 0);
	mask = inputGen.GetLine(width, 0);

	mask[23] = 1;
	inputLines[0][23] = 7;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(7, asmMinVal);
	EXPECT_EQ(7, asmMaxVal);
	EXPECT_EQ(23, asmMinPos);
	EXPECT_EQ(23, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAllValuesUnmasked) 
{
	width = 640;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 0);
	mask = inputGen.GetLine(width, 0);

	inputLines[0][230] = 7;
	inputLines[0][630] = 220;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(255, asmMinVal);
	EXPECT_EQ(0, asmMaxVal);
	EXPECT_EQ(0, asmMinPos);
	EXPECT_EQ(0, asmMaxPos);
}

TEST_F(MinMaxPosTest, TestAlternateMaskValues)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 8);
	mask = inputGen.GetLine(width, 0);

	for(int i = 0; i < width; i++) {
		if(i % 2 == 0) {
			mask[i] = 1;
		}
	}

	inputLines[0][8] = 9;
	inputLines[0][9] = 22;
	inputLines[0][10] = 6;
	inputLines[0][11] = 40;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(6, asmMinVal);
	EXPECT_EQ(9, asmMaxVal);
	EXPECT_EQ(10, asmMinPos);
	EXPECT_EQ(8, asmMaxPos);
}


TEST_F(MinMaxPosTest, TestRandomDataSize8)
{
	width = 8;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos,mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;

}

TEST_F(MinMaxPosTest, TestRandomDataSize320)
{
	unsigned int width = 64;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos,mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;

}

TEST_F(MinMaxPosTest, TestRandomDataSize648)
{
	width = 648;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos, mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;
}

TEST_F(MinMaxPosTest, TestRandomDataSize800)
{
	width = 800;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos, mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;
}

TEST_F(MinMaxPosTest, TestRandomDataSize1280)
{
	width = 1280;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos, mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;
}

TEST_F(MinMaxPosTest, TestRandomDataSize1920)
{
	width = 1920;
	inputGen.SelectGenerator("random");
	inputLines = inputGen.GetLines(width, 1, 0, 255);
	mask = inputGen.GetLine(width, 0, 2);

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);
	minMaxPos(inputLines, width, &cMinVal, &cMaxVal, &cMinPos, &cMaxPos, mask);

	EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
	EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
	EXPECT_TRUE(cMinPos == asmMinPos) << "cMinPos: " << (unsigned int)cMinPos << " asmMinPos: " << (unsigned int)asmMinPos;
	EXPECT_TRUE(cMaxPos == asmMaxPos) << "cMaxPos: " << (unsigned int)cMaxPos << " asmMaxPos: " << (unsigned int)asmMaxPos;
}

TEST_F(MinMaxPosTest, TestMinimumLineWidth)
{

	width = 8;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 15);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][7] = 16;
	inputLines[0][3] = 0;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(0, asmMinVal);
	EXPECT_EQ(16, asmMaxVal);
	EXPECT_EQ(3, asmMinPos);
	EXPECT_EQ(7, asmMaxPos);
}


TEST_F(MinMaxPosTest, TestLineWidthLowerThan16)
{

	width = 8;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 15);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][7] = 16;
	inputLines[0][3] = 0;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(0, asmMinVal);
	EXPECT_EQ(16, asmMaxVal);
	EXPECT_EQ(3, asmMinPos);
	EXPECT_EQ(7, asmMaxPos);
}


TEST_F(MinMaxPosTest, TestUniformInputNotMultipleOf16)
{

	width = 24;
	inputGen.SelectGenerator("uniform");
	inputLines = inputGen.GetLines(width, 1, 15);
	mask = inputGen.GetLine(width, 1);

	inputLines[0][5] = 1;  mask[5] = 0;
	inputLines[0][4] = 20;
	inputLines[0][17] = 21;
	inputLines[0][22] = 1;

	minMaxPos_asm_test(inputLines, width, &asmMinVal, &asmMaxVal, &asmMinPos, &asmMaxPos, mask);
	RecordProperty("CycleCount", minMaxCycleCountPos);

	EXPECT_EQ(1, asmMinVal);
	EXPECT_EQ(21, asmMaxVal);
	EXPECT_EQ(22, asmMinPos);
	EXPECT_EQ(17, asmMaxPos);
}
