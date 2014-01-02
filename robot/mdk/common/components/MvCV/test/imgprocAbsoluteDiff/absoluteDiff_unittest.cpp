//absoluteDiff test
//Asm function prototype:
//    void AbsoluteDiff_asm(u8** in1, u8** in2, u8** out, u32 width);
//
//Asm test function prototype:
//    void absoluteDiff_asm_test(unsigned char** in1, unsigned char** in2, unsigned char** out, unsigned int width);
//
//C function prototype:
//    void AbsoluteDiff(u8** in1, u8** in2, u8** out, u32 width);
//
//Parameter description:
// in1     - array of pointers to input lines of the first image
// in2     - array of pointers to input lines of the second image
// out     - array of pointers to output lines
// width   - width of the input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "absoluteDiff_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

using namespace mvcv;

class AbsoluteDiffTest : public ::testing::Test {
 protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}


	void AlternativeCompare(unsigned int var1, unsigned int var2, u8** out, unsigned int length)
		{
			for(int i=0;i<length-2;i+=2)
			{
				EXPECT_EQ(var1, out[0][i]);
			}
			for(int i=1;i<length-2;i+=2)
			{
				EXPECT_EQ(var2, out[0][i]);
			}
		}



    u8** inLines1;
    u8** inLines2;
    u8** outLinesC;
    u8** outLinesASM;

    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};

TEST_F(AbsoluteDiffTest , TestUniformInputLineNegDiff)
{

	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(328, 1, 1);
	inLines2 = inputGen.GetLines(328, 1, 4);
	outLinesC = inputGen.GetEmptyLines(328, 1);
	outLinesASM = inputGen.GetEmptyLines(328, 1);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 328);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 328);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],328,3);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],328);
}

TEST_F(AbsoluteDiffTest , TestUniformInputLinePosDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(640, 1, 3);
	inLines2 = inputGen.GetLines(640, 1, 1);
	outLinesC = inputGen.GetEmptyLines(640, 1);
	outLinesASM = inputGen.GetEmptyLines(640, 1);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 640);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 640);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesC[0],640,2);
	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],640,2);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],640);
}

TEST_F(AbsoluteDiffTest , TestUniformInputLineNumMinusZero)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(960, 1, 4);
	inLines2 = inputGen.GetLines(960, 1, 0);
	outLinesC = inputGen.GetEmptyLines(960, 1);
	outLinesASM = inputGen.GetEmptyLines(960, 1);

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 960);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 960);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],960,4);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],960);
}

TEST_F(AbsoluteDiffTest , TestUniformInputLineZeroDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(8, 1, 3);
	inLines2 = inputGen.GetLines(8, 1, 3);
	outLinesC = inputGen.GetEmptyLines(8, 1);
	outLinesASM = inputGen.GetEmptyLines(8, 1);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 8);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 8);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],8,0);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],8);
}


TEST_F(AbsoluteDiffTest , TestAlterInputLineNumMinusNum)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(32, 1, 3);
	inLines2 = inputGen.GetLines(32, 1, 3);
	outLinesC = inputGen.GetEmptyLines(32, 1);
	outLinesASM = inputGen.GetEmptyLines(32, 1);
	for(int i = 0; i < 32; i+=2)
	{
		inLines2[0][i]=1;
	}

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 32);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 32);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	AlternativeCompare(2,0,outLinesASM,32);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],32);

}


TEST_F(AbsoluteDiffTest , TestUniformInputLineMaxMinusZeroDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(1744, 1, 255);
	inLines2 = inputGen.GetLines(1744, 1, 0);
	outLinesC = inputGen.GetEmptyLines(1744, 1);
	outLinesASM = inputGen.GetEmptyLines(1744, 1);

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 1744);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 1744);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],1744,255);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],1744);
}


TEST_F(AbsoluteDiffTest , TestUniformInputLineNumMinusMaxDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(104, 1, 3);
	inLines2 = inputGen.GetLines(104, 1, 255);
	outLinesC = inputGen.GetEmptyLines(104, 1);
	outLinesASM = inputGen.GetEmptyLines(104, 1);

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 104);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 104);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],104,252);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],104);
}


TEST_F(AbsoluteDiffTest , TestUniformInputLineMaxMinusMaxDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(256, 1, 255);
	inLines2 = inputGen.GetLines(256, 1, 255);
	outLinesC = inputGen.GetEmptyLines(256, 1);
	outLinesASM = inputGen.GetEmptyLines(256, 1);

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 256);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 256);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],256,0);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],256);
}


TEST_F(AbsoluteDiffTest , TestZeroMiusZeroDiff)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(512, 1, 0);
	inLines2 = inputGen.GetLines(512, 1, 0);
	outLinesC = inputGen.GetEmptyLines(512, 1);
	outLinesASM = inputGen.GetEmptyLines(512, 1);

	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 512);
	AbsoluteDiff( inLines1, inLines2, outLinesC, 512);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	arrCheck.ExpectAllEQ<unsigned char>(outLinesASM[0],512,0);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],512);
}


TEST_F(AbsoluteDiffTest , TestAlterDiffMaxMinusZeroAndMax)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(1024, 1, 255);
	inLines2 = inputGen.GetLines(1024, 1, 255);
	outLinesC = inputGen.GetEmptyLines(1024, 1);
	outLinesASM = inputGen.GetEmptyLines(1024, 1);
	for(int i = 0; i < 1024; i+=2)
	{
		inLines2[0][i]=0;
	}


	AbsoluteDiff( inLines1, inLines2, outLinesC, 1024);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 1024);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	AlternativeCompare(255,0,outLinesASM,1024);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],1024);
}


TEST_F(AbsoluteDiffTest , TestAlterDiffZeroAndMaxMinusMax)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(16, 1, 255);
	inLines2 = inputGen.GetLines(16, 1, 255);
	outLinesC = inputGen.GetEmptyLines(16, 1);
	outLinesASM = inputGen.GetEmptyLines(16, 1);
	for(int i = 1; i < 16; i+=2)
	{
		inLines1[0][i]=0;
	}


	AbsoluteDiff( inLines1, inLines2, outLinesC, 16);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 16);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	AlternativeCompare(0,255,outLinesASM,16);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],16);

}


TEST_F(AbsoluteDiffTest , TestAlterDiffNumAndMaxMinusMax)
{
	inputGen.SelectGenerator("uniform");
	inLines1 = inputGen.GetLines(224, 1, 255);
	inLines2 = inputGen.GetLines(224, 1, 255);
	outLinesC = inputGen.GetEmptyLines(224, 1);
	outLinesASM = inputGen.GetEmptyLines(224, 1);
	for(int i = 0; i < 224; i+=2)
	{
		inLines1[0][i]=3;
	}

	AbsoluteDiff( inLines1, inLines2, outLinesC, 224);
	AbsoluteDiff_asm_test( inLines1, inLines2, outLinesASM, 224);
	RecordProperty("CycleCount", AbsoluteDiffCycleCount);

	AlternativeCompare(252,0,outLinesASM,224);
	arrCheck.CompareArrays(outLinesASM[0],outLinesC[0],224);
}
