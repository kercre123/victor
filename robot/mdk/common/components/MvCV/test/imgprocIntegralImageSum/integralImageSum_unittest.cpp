//IntegralImageSumTest
//Asm function prototype:
//   void integralimage_sum_u32_asm(u8** in, u8** out, u32* sum, u32 width);
//
//Asm test function prototype:
//   void integralImageSum_asm_test(u8** in, u8** out, u32* sum, u32 width);
//
//C function prototype:
//   void integralimage_sum_u32(u8** in, u8** out, u32* sum, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// sum       - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line		  
// width     - width of input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "core.h"
#include "integralImageSum_asm_test.h"
#include "InputGenerator.h"
#include "RandomGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;


class IntegralImageSumTest : public ::testing::Test {
protected:

virtual void SetUp()
{
    randGen.reset(new RandomGenerator);
    uniGen.reset(new UniformGenerator);
    inputGen.AddGenerator(std::string("random"), randGen.get());
    inputGen.AddGenerator(std::string("uniform"), uniGen.get());
}

    unsigned int width;
    InputGenerator inputGen;
    RandomGenerator ranGen;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;
    unsigned char** inLines;
    unsigned char** outLinesAsm;
    unsigned char** outLinesC;
    long unsigned int *asmSum;
    long unsigned int *cSum;
    ArrayChecker outputChecker;


    virtual void TearDown()
    {
    }
};

TEST_F(IntegralImageSumTest, TestAsmAllValues0)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(0, outAsm[319]);
    EXPECT_EQ(0, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSumTest, TestAsmLowestLineSize)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 1, 0);
    
    inLines[0][0] = 6;
    inLines[0][15] = 10;
    
    outLinesAsm = inputGen.GetEmptyLines(16 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(16 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, 16);
    integralimage_sum_u32(inLines, outLinesC, cSum, 16);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(6, outAsm[0]);
    EXPECT_EQ(6, outC[0]);
    EXPECT_EQ(16, outAsm[15]);
    EXPECT_EQ(16, outC[15]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 16 * 4);
}

TEST_F(IntegralImageSumTest, TestAsmFixedValues)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    inLines[0][5] = 100;
    inLines[0][200] = 2;
    
    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(0, outAsm[4]);
    EXPECT_EQ(0, outC[4]);
    EXPECT_EQ(100, outAsm[5]);
    EXPECT_EQ(100, outC[5]);
    EXPECT_EQ(100, outAsm[199]);
    EXPECT_EQ(100, outC[199]);
    EXPECT_EQ(102, outAsm[200]);
    EXPECT_EQ(102, outC[200]);
    EXPECT_EQ(102, outAsm[319]);
    EXPECT_EQ(102, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSumTest, TestAsmAllValuesOne)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 1);

    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(320, outAsm[319]);
    EXPECT_EQ(320, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSumTest, TestAsmAllValues255)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 255);

    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(320 * 255, outAsm[319]);
    EXPECT_EQ(320 * 255, outC[319]);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSumTest, TestWidth240RandomData)
{
    width = 240;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);
 
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSumTest, TestWidth320RandomData)
{
    width = 320;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);
  
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);
  
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSumTest, TestWidth640RandomData)
{
    width = 640;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSumTest, TestWidth1280RandomData)
{
    width = 1280;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSumTest, TestWidth1920RandomData)
{
    width = 1920;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);
    
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSumTest, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = ranGen.GenerateUInt(0, 1920, 16);
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumCycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSumTest, TestAsmTwoLines)
{
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(320, 2, 0);

    inLines[0][0] = 5;
    inLines[0][1] = 2;
    inLines[0][2] = 3;
    inLines[0][3] = 4;
    inLines[0][4] = 1;
    inLines[1][0] = 1;
    inLines[1][1] = 5;
    inLines[1][2] = 4;
    inLines[1][3] = 2;
    inLines[1][4] = 3;
    
    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 2);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 2);
    
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSum_asm_test(&inLines[0], &outLinesAsm[0], asmSum, 320);
    integralImageSum_asm_test(&inLines[1], &outLinesAsm[1], asmSum, 320);

    integralimage_sum_u32(&inLines[0], &outLinesC[0], cSum, 320);
    integralimage_sum_u32(&inLines[1], &outLinesC[1], cSum, 320);

    RecordProperty("CycleCount", integralImageSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];

    EXPECT_EQ(5, outAsm[0]);
    EXPECT_EQ(7, outAsm[1]);
    EXPECT_EQ(10, outAsm[2]);
    EXPECT_EQ(14, outAsm[3]);
    EXPECT_EQ(15, outAsm[4]);
    
    outAsm = (u32*)outLinesAsm[1];
    
    EXPECT_EQ(6, outAsm[0]);
    EXPECT_EQ(13, outAsm[1]);
    EXPECT_EQ(20, outAsm[2]);
    EXPECT_EQ(26, outAsm[3]);
    EXPECT_EQ(30, outAsm[4]);
   
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
    outputChecker.CompareArrays(outLinesC[1], outLinesAsm[1], 320 * 4);

}
