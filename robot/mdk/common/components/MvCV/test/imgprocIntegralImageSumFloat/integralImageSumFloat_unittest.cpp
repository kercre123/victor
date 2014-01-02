//IntegralImageSumFloatTest
//Asm function prototype:
//   void integralimage_sum_f32_asm(u8** in, u8** out, u32* sum, u32 width);
//
//Asm test function prototype:
//   void integralImageSumF_asm_test(u8** in, u8** out, u32* sum, u32 width);
//
//C function prototype:
//   void integralimage_sum_f32(u8** in, u8** out, u32* sum, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// sum       - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
// width     - width of input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "core.h"
#include "integralImageSumFloat_asm_test.h"
#include "InputGenerator.h"
#include "RandomGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;


class IntegralImageSumFloatTest : public ::testing::Test {
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
    float** outLinesAsm;
    float** outLinesC;
    long unsigned int *asmSum;
    long unsigned int *cSum;
    ArrayChecker outputChecker;

    virtual void TearDown()
    {
    }
};

TEST_F(IntegralImageSumFloatTest, TestAsmAllValues0)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);

    outLinesAsm = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, 320);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = (fp32*)outLinesAsm[0];
    fp32* outC = (fp32*)outLinesC[0];

    EXPECT_FLOAT_EQ(0.0F, outAsm[319]);
    EXPECT_FLOAT_EQ(0.0F, outC[319]);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(IntegralImageSumFloatTest, TestAsmLowestLineSize)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 1, 0);

    inLines[0][0] = 6;
    inLines[0][15] = 10;
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(16 * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(16 * sizeof(float), 1);

    asmSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(16 * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, 16);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, 16);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = outLinesAsm[0];
    fp32* outC = outLinesC[0];

    EXPECT_FLOAT_EQ(6.0F, outAsm[0]);
    EXPECT_FLOAT_EQ(6.0F, outC[0]);
    EXPECT_FLOAT_EQ(16.0F, outAsm[15]);
    EXPECT_FLOAT_EQ(16.0F, outC[15]);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 16);
}

TEST_F(IntegralImageSumFloatTest, TestAsmFixedValues)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    inLines[0][5] = 100;
    inLines[0][200] = 2;
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * sizeof(float), 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * sizeof(float), 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, 320);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = (fp32*)outLinesAsm[0];
    fp32* outC = (fp32*)outLinesC[0];

    EXPECT_FLOAT_EQ(0.0F, outAsm[4]);
    EXPECT_FLOAT_EQ(0.0F, outC[4]);
    EXPECT_FLOAT_EQ(100.0F, outAsm[5]);
    EXPECT_FLOAT_EQ(100.0F, outC[5]);
    EXPECT_FLOAT_EQ(100.0F, outAsm[199]);
    EXPECT_FLOAT_EQ(100.0F, outC[199]);
    EXPECT_FLOAT_EQ(102.0F, outAsm[200]);
    EXPECT_FLOAT_EQ(102.0F, outC[200]);
    EXPECT_FLOAT_EQ(102.0F, outAsm[319]);
    EXPECT_FLOAT_EQ(102.0F, outC[319]);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(IntegralImageSumFloatTest, TestAsmAllValuesOne)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 1);
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, 320);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = (fp32*)outLinesAsm[0];
    fp32* outC = (fp32*)outLinesC[0];

    EXPECT_FLOAT_EQ(320.0F, outAsm[319]);
    EXPECT_FLOAT_EQ(320.0F, outC[319]);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(IntegralImageSumFloatTest, TestAsmAllValues255)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 255);

    outLinesAsm = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 1);

    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, 320);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, 320);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = (fp32*)outLinesAsm[0];
    fp32* outC = (fp32*)outLinesC[0];

    EXPECT_FLOAT_EQ(81600.0F, outAsm[319]);
    EXPECT_FLOAT_EQ(81600.0F, outC[319]);
    
    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 320);
}
TEST_F(IntegralImageSumFloatTest, TestWidth240RandomData)
{
    width = 240;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);
 
    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width); 
}

TEST_F(IntegralImageSumFloatTest, TestWidth320RandomData)
{
    width = 320;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);
  
    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width); 
}

TEST_F(IntegralImageSumFloatTest, TestWidth640RandomData)
{
    width = 640;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(IntegralImageSumFloatTest, TestWidth1280RandomData)
{
    width = 1280;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);

    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(IntegralImageSumFloatTest, TestWidth1920RandomData)
{
    width = 1920;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
   
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);

    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width); 
}

TEST_F(IntegralImageSumFloatTest, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = ranGen.GenerateUInt(0, 1920, 16);
    inLines = inputGen.GetLines(width, 1, 0, 255);
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
    outLinesC = (float**)inputGen.GetEmptyLines(width * sizeof(float), 1);
   
    inputGen.SelectGenerator("uniform");
    asmSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(width * 4, 0);

    integralImageSumF_asm_test(inLines, (unsigned char**)outLinesAsm, asmSum, width);
    integralimage_sum_f32(inLines, (unsigned char**)outLinesC, cSum, width);
    RecordProperty("CycleCount", integralImageSumFCycleCount);
    
    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(IntegralImageSumFloatTest, TestAsmTwoLines)
{
    inputGen.SelectGenerator("uniform");
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
    
    outLinesAsm = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 2);
    outLinesC = (float**)inputGen.GetEmptyLines(320 * sizeof(float), 2);
        
    asmSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);
    cSum = (long unsigned int *)inputGen.GetLine(320 * 4, 0);

    integralImageSumF_asm_test(&inLines[0], (unsigned char**)&outLinesAsm[0], asmSum, 320);
    integralImageSumF_asm_test(&inLines[1], (unsigned char**)&outLinesAsm[1], asmSum, 320);

    integralimage_sum_f32(&inLines[0], (unsigned char**)&outLinesC[0], cSum, 320);
    integralimage_sum_f32(&inLines[1], (unsigned char**)&outLinesC[1], cSum, 320);

    RecordProperty("CycleCount", integralImageSumFCycleCount);

    fp32* outAsm = (fp32*)outLinesAsm[0];
   
    EXPECT_FLOAT_EQ(5.0F, outAsm[0]);
    EXPECT_FLOAT_EQ(7.0F, outAsm[1]);
    EXPECT_FLOAT_EQ(10.0F, outAsm[2]);
    EXPECT_FLOAT_EQ(14.0F, outAsm[3]);
    EXPECT_FLOAT_EQ(15.0F, outAsm[4]);

    outAsm = (fp32*)outLinesAsm[1];

    EXPECT_FLOAT_EQ(6.0F, outAsm[0]);
    EXPECT_FLOAT_EQ(13.0F, outAsm[1]);
    EXPECT_FLOAT_EQ(20.0F, outAsm[2]);
    EXPECT_FLOAT_EQ(26.0F, outAsm[3]);
    EXPECT_FLOAT_EQ(30.0F, outAsm[4]);
   
    outputChecker.CompareArrays<float>(outLinesC[0], outLinesAsm[0], 320);
    outputChecker.CompareArrays<float>(outLinesC[1], outLinesAsm[1], 320);

}
