//minMaxKernel test
//Asm function prototype:
//        void meanstddev_asm(u8** in, float *mean, float *stddev, u32 width);
//
//Asm test function prototype:
//   void meanstddev_asm_test(u8** in, float *mean, float *stddev, unsigned int width);
//
//C function prototype:
//    void meanstddev(u8** in, float *mean, float *stddev, u32 width);
//
//Parameter description:
// in the source array
// mean computed mean value
// stddev computed standard deviation
// width array size

#include "gtest/gtest.h"
#include "core.h"
#include "meanStdDev_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

#define margin 0.05

using namespace mvcv;

class meanstddevTest : public ::testing::Test {
 protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
		meanASM=0;
		stdevASM=0;

		meanC=0;
		stdevC=0;
	}



    u8** inLines;

    float meanC,stdevC,meanASM,stdevASM;

    unsigned int width;

    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};

TEST_F(meanstddevTest , TestIncrementalInputLine1)
{

	width=32;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(width, 1);
	int i;
	for(i=0;i<width;i++)
	{
		inLines[0][i]=i+1;
	}

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(16.5,meanC,margin);
	EXPECT_NEAR(16.5,meanASM,margin);

	EXPECT_NEAR(9.23309265630969,stdevC,margin);
	EXPECT_NEAR(9.23309265630969,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);
}

TEST_F(meanstddevTest , TestIncrementalInputLine2)
{

	width=144;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(width, 1);
	int i;
	for(i=0;i<width;i++)
	{
		inLines[0][i]=i+1;
	}

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(72.5,meanC,margin);
	EXPECT_NEAR(72.5,meanASM,margin);

	EXPECT_NEAR(41.5682170253508,stdevC,margin);
	EXPECT_NEAR(41.5682170253508,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);
}

TEST_F(meanstddevTest , TestUniformInputLine1)
{
	width=104;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 1);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount" ,meanstddevCycleCount);

	EXPECT_NEAR(1,meanC,margin);
	EXPECT_NEAR(1,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}



TEST_F(meanstddevTest , TestNullInputLine)
{
	width=128;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 0);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(0,meanC,margin);
	EXPECT_NEAR(0,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}

TEST_F(meanstddevTest , TestOneNonNullInputLine)
{
	width=328;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 0);

	inLines[0][16]=200;

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(0.609756097560976,meanC,margin);
	EXPECT_NEAR(0.609756097560976,meanASM,margin);

	EXPECT_NEAR(11.0263056829422,stdevC,margin);
	EXPECT_NEAR(11.0263056829422,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}

TEST_F(meanstddevTest , TestUniformHighValuInputLine)
{
	width=256;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 255);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(255,meanC,margin);
	EXPECT_NEAR(255,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}

TEST_F(meanstddevTest , TestUniformInputLineOverflow)
{
	width=256;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 256);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(0,meanC,margin);
	EXPECT_NEAR(0,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}



TEST_F(meanstddevTest , TestDiverseInputLine12)
{
	width=32;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(width, 1);

	inLines[0][0]=100;
	inLines[0][1]=200;
	inLines[0][2]=250;
	inLines[0][3]=150;
	inLines[0][4]=50;
	inLines[0][5]=100;
	inLines[0][6]=200;
	inLines[0][7]=250;
	inLines[0][8]=100;
	inLines[0][9]=200;
	inLines[0][10]=250;
	inLines[0][11]=150;
	inLines[0][12]=50;
	inLines[0][13]=100;
	inLines[0][14]=200;
	inLines[0][15]=250;
	inLines[0][16]=100;
	inLines[0][17]=200;
	inLines[0][18]=250;
	inLines[0][19]=150;
	inLines[0][20]=50;
	inLines[0][21]=100;
	inLines[0][22]=200;
	inLines[0][23]=250;
	inLines[0][24]=100;
	inLines[0][25]=200;
	inLines[0][26]=250;
	inLines[0][27]=150;
	inLines[0][28]=50;
	inLines[0][29]=100;
	inLines[0][30]=200;
	inLines[0][31]=250;

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(162.5,meanC,margin);
	EXPECT_NEAR(162.5,meanASM,margin);

	EXPECT_NEAR(69.5970545353753,stdevC,margin);
	EXPECT_NEAR(69.5970545353753,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}


TEST_F(meanstddevTest , TestUniformInputLineLowWidthMaxInput)
{
	width=648;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 255);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(255,meanC,margin);
	EXPECT_NEAR(255,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}


TEST_F(meanstddevTest , TestRandomInputLine)
{

	width=168;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetEmptyLines(width, 1);
	int i;
	for(i=0;i<width;i++)
	{
		inLines[0][i]=dataGenerator.GenerateUInt(0,255,1);
	}

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);
}

TEST_F(meanstddevTest , TestUniformHighWidthInputLine1)
{
	width=1912;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 200);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(200,meanC,margin);
	EXPECT_NEAR(200,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}

TEST_F(meanstddevTest , TestUniformHighWidthInputLine2)
{
	width=1024;

	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 255);

	meanstddev_asm_test( inLines, &meanASM, &stdevASM, width);

	meanstddev( inLines, &meanC, &stdevC, width);

	RecordProperty("CycleCount", meanstddevCycleCount);

	EXPECT_NEAR(255,meanC,margin);
	EXPECT_NEAR(255,meanASM,margin);

	EXPECT_NEAR(0,stdevC,margin);
	EXPECT_NEAR(0,stdevASM,margin);

	EXPECT_NEAR(meanC,meanASM,margin);
	EXPECT_NEAR(stdevC,stdevASM,margin);


}


