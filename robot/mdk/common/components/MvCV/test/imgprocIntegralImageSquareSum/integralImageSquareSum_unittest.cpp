//IntegralImageSquareSumTest
//Asm function prototype:
//   void integralimage_sqsum_u32_asm(u8** in, u8** out, u32* sum, u32 width);
//
//Asm test function prototype:
//   void integralImageSquareSum_asm_test(u8** in, u8** out, u32* sum, u32 width);
//
//C function prototype:
//   void integralimage_sqsum_u32(u8** in, u8** out, u32* sum, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// sum       - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
//             (this particular case makes square sum of pixels)
// width     - width of input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "core.h"
#include "integralImageSquareSum_asm_test.h"
#include "InputGenerator.h"
#include "RandomGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;


class IntegralImageSquareSumTest : public ::testing::Test {
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

TEST_F(IntegralImageSquareSumTest, TestAsmAllValues0)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);

    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(0, outAsm[319]);
    EXPECT_EQ(0, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSquareSumTest, TestAsmLowestLineSize)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 1, 0);

    inLines[0][0] = 6;
    inLines[0][15] = 10;
    
    outLinesAsm = inputGen.GetEmptyLines(16 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(16 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, 16);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, 16);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(36, outAsm[0]);
    EXPECT_EQ(36, outC[0]);
    EXPECT_EQ(136, outAsm[15]);
    EXPECT_EQ(136, outC[15]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 16 * 4);
}

TEST_F(IntegralImageSquareSumTest, TestAsmFixedValues)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    inLines[0][5] = 100;
    inLines[0][200] = 2;
    
    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(0, outAsm[4]);
    EXPECT_EQ(0, outC[4]);
    EXPECT_EQ(10000, outAsm[5]);
    EXPECT_EQ(10000, outC[5]);
    EXPECT_EQ(10000, outAsm[199]);
    EXPECT_EQ(10000, outC[199]);
    EXPECT_EQ(10004, outAsm[200]);
    EXPECT_EQ(10004, outC[200]);
    EXPECT_EQ(10004, outAsm[319]);
    EXPECT_EQ(10004, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSquareSumTest, TestAsmAllValuesOne)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 1);
    
    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(320, outAsm[319]);
    EXPECT_EQ(320, outC[319]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSquareSumTest, TestAsmAllValues255)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 255);

    outLinesAsm = inputGen.GetEmptyLines(320 * 4, 1);
    outLinesC = inputGen.GetEmptyLines(320 * 4, 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, 320);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
    u32* outC = (u32*)outLinesC[0];

    EXPECT_EQ(320 * 255 * 255, outAsm[319]);
    EXPECT_EQ(320 * 255 * 255, outC[319]);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
}

TEST_F(IntegralImageSquareSumTest, TestWidth240RandomData)
{
    width = 240;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);
 
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSquareSumTest, TestWidth320RandomData)
{
    width = 320;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);
  
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSquareSumTest, TestWidth640RandomData)
{
    width = 640;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);
   
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSquareSumTest, TestWidth1280RandomData)
{
    width = 1280;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSquareSumTest, TestWidth1920RandomData)
{
    width = 1920;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);
   
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4); 
}

TEST_F(IntegralImageSquareSumTest, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = ranGen.GenerateUInt(0, 1920, 16);
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);
   
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSquareSum_asm_test(inLines, outLinesAsm, asmSum, width);
    integralimage_sqsum_u32(inLines, outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSquareSumCycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width * 4);
}

TEST_F(IntegralImageSquareSumTest, TestAsmTwoLines)
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

    integralImageSquareSum_asm_test(&inLines[0], &outLinesAsm[0], asmSum, 320);
    integralImageSquareSum_asm_test(&inLines[1], &outLinesAsm[1], asmSum, 320);

    integralimage_sqsum_u32(&inLines[0], &outLinesC[0], cSum, 320);
    integralimage_sqsum_u32(&inLines[1], &outLinesC[1], cSum, 320);

    RecordProperty("CycleCount", integralImageSquareSumCycleCount);

    u32* outAsm = (u32*)outLinesAsm[0];
   
    EXPECT_EQ(25, outAsm[0]);
    EXPECT_EQ(29, outAsm[1]);
    EXPECT_EQ(38, outAsm[2]);
    EXPECT_EQ(54, outAsm[3]);
    EXPECT_EQ(55, outAsm[4]);

    outAsm = (u32*)outLinesAsm[1];

    EXPECT_EQ(26, outAsm[0]);
    EXPECT_EQ(55, outAsm[1]);
    EXPECT_EQ(80, outAsm[2]);
    EXPECT_EQ(100, outAsm[3]);
    EXPECT_EQ(110, outAsm[4]);
   
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320 * 4);
    outputChecker.CompareArrays(outLinesC[1], outLinesAsm[1], 320 * 4);

}
