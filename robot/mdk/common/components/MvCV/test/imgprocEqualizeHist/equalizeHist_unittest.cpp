//EqualizeHistTest
//Asm function prototype:
//   void equalizeHist_asm(u8** in, u8** out, u32 *hist, u32 width);
//
//Asm test function prototype:
//   void equalizeHist_asm_test(u8** in, u8** out, u32 *hist, u32 width);
//
//C function prototype:
//   void equalizeHist(u8** in, u8** out, u32 *hist, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// hist      - pointer to an input array that indicates the cumulative histogram of the image
// width     - width of input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "core.h"
#include "equalizeHist_asm_test.h"
#include "InputGenerator.h"
#include "RandomGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;


class EqualizeHistTest : public ::testing::Test {
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
    long unsigned int *asmHist;
    long unsigned int *cHist;
    ArrayChecker outputChecker;

    void cumulateHist(long unsigned cHist[256], int width)
    {
        for( int i = 1; i <= 256; i++)
            cHist[i] = cHist[i-1] + cHist[i];
    }
    
    virtual void TearDown()
    {
    }
};

TEST_F(EqualizeHistTest, TestAsmAllValues0)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);

    outLinesAsm = inputGen.GetLines(320 * 4, 1, 0);
    outLinesC = inputGen.GetLines(320* 4, 1, 0);
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram(inLines, asmHist, 320);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, 320);
    equalizeHist(inLines, outLinesC, cHist, 320);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);
        
    EXPECT_EQ(255, outLinesC[0][0]);
    EXPECT_EQ(255, outLinesC[0][100]);
    EXPECT_EQ(255, outLinesC[0][319]);
    EXPECT_EQ(255, outLinesAsm[0][0]);
    EXPECT_EQ(255, outLinesAsm[0][100]);
    EXPECT_EQ(255, outLinesAsm[0][319]);
        
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);
}

TEST_F(EqualizeHistTest, TestAsmAllValues255)
{
    int j;
    
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);

    for( j = 0; j < 320; j++)
    {
        inLines[0][j] = 255;
    }
    
    outLinesAsm = inputGen.GetLines(320 * 4, 1, 0);
    outLinesC = inputGen.GetLines(320* 4, 1, 0);
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram(inLines, asmHist, 320);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, 320);
    equalizeHist(inLines, outLinesC, cHist, 320);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    EXPECT_EQ(255, outLinesC[0][0]);
    EXPECT_EQ(255, outLinesC[0][100]);
    EXPECT_EQ(255, outLinesC[0][319]);
    EXPECT_EQ(255, outLinesAsm[0][0]);
    EXPECT_EQ(255, outLinesAsm[0][100]);
    EXPECT_EQ(255, outLinesAsm[0][319]);
       
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);  
}

TEST_F(EqualizeHistTest, TestAsmAllValuesOne)
{
    int j;
    
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);

    for( j = 0; j < 320; j++)
    {
        inLines[0][j] = 1;
    }
    
    outLinesAsm = inputGen.GetLines(320 * 4, 1, 0);
    outLinesC = inputGen.GetLines(320* 4, 1, 0);
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram(inLines, asmHist, 320);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, 320);
    equalizeHist(inLines, outLinesC, cHist, 320);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);
        
    EXPECT_EQ(255, outLinesC[0][0]);
    EXPECT_EQ(255, outLinesC[0][100]);
    EXPECT_EQ(255, outLinesC[0][319]);
    EXPECT_EQ(255, outLinesAsm[0][0]);
    EXPECT_EQ(255, outLinesAsm[0][100]);
    EXPECT_EQ(255, outLinesAsm[0][319]);
       
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 320);  
}


TEST_F(EqualizeHistTest, TestAsmLowestLineSize)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 1, 0);
    
    inLines[0][0] = 1;
    inLines[0][1] = 18;
    inLines[0][2] = 3;
    inLines[0][3] = 25;
    inLines[0][4] = 78;
    inLines[0][5] = 100;
    inLines[0][6] = 10;
    inLines[0][7] = 1;
    inLines[0][8] = 2;
    inLines[0][9] = 2;
    inLines[0][10] = 200;
    inLines[0][11] = 5;
    inLines[0][12] = 5;
    inLines[0][13] = 2;
    inLines[0][14] = 120;
    inLines[0][15] = 119;
    
    outLinesAsm = inputGen.GetLines(16 * 4, 1, 0);
    outLinesC = inputGen.GetLines(16 * 4, 1, 0);
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 16);
    histogram(inLines, asmHist, 16);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, 16);
    equalizeHist(inLines, outLinesC, cHist, 16);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    EXPECT_EQ(2, cHist[1]);
    EXPECT_EQ(10, cHist[18]);
    EXPECT_EQ(16, cHist[255]);
    EXPECT_EQ(2, asmHist[1]);
    EXPECT_EQ(10, asmHist[18]);
    EXPECT_EQ(16, asmHist[255]);
    EXPECT_EQ(31, outLinesAsm[0][0]);
    EXPECT_EQ(159, outLinesAsm[0][1]);
    EXPECT_EQ(223, outLinesAsm[0][15]);
    EXPECT_EQ(31, outLinesC[0][0]);
    EXPECT_EQ(159, outLinesC[0][1]);
    EXPECT_EQ(223, outLinesC[0][15]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 16);
}

TEST_F(EqualizeHistTest, TestWidth240RandomData)
{
    width = 240;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);  
}

TEST_F(EqualizeHistTest, TestWidth2320RandomData)
{
    width = 320;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);  
}

TEST_F(EqualizeHistTest, TestWidth640RandomData)
{
    width = 640;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);  
}

TEST_F(EqualizeHistTest, TestWidth1280RandomData)
{
    width = 1280;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);  
}

TEST_F(EqualizeHistTest, TestWidth1920RandomData)
{
    width = 1920;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);  
}

TEST_F(EqualizeHistTest, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = ranGen.GenerateUInt(0, 1920, 16);
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    outLinesAsm = inputGen.GetEmptyLines(width * 4, 1);
    outLinesC = inputGen.GetEmptyLines(width * 4, 1);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram(inLines, asmHist, width);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(inLines, outLinesAsm, asmHist, width);
    equalizeHist(inLines, outLinesC, cHist, width);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width, 1);  
}

TEST_F(EqualizeHistTest, TestAsmTwoLines)
{
    int j;
    
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 2, 0);
  
    inLines[0][0] = 5;
    inLines[0][1] = 2;
    inLines[0][2] = 3;
    inLines[1][1] = 5;
    inLines[1][2] = 4;
    
    outLinesAsm = inputGen.GetEmptyLines(16 * 4, 2);
    outLinesC = inputGen.GetEmptyLines(16 * 4, 2);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(&inLines[0], cHist, 16);
    histogram(&inLines[0], asmHist, 16);
    histogram(&inLines[1], cHist, 16);
    histogram(&inLines[1], asmHist, 16);
    cumulateHist(cHist, 16);
    cumulateHist(asmHist, 16);
    
    equalizeHist_asm_test(&inLines[0], &outLinesAsm[0], asmHist, 16);
    equalizeHist_asm_test(&inLines[1], &outLinesAsm[1], asmHist, 16);
    equalizeHist(&inLines[0],  &outLinesC[0], cHist, 16);
    equalizeHist(&inLines[1],  &outLinesC[1], cHist, 16);
    
    RecordProperty("CycleCount", equalizeHistCycleCount);
 
    EXPECT_EQ(27, cHist[0]);
    EXPECT_EQ(27, asmHist[0]);
    EXPECT_EQ(32, cHist[255]);
    EXPECT_EQ(32, asmHist[255]);
    EXPECT_EQ(255, outLinesAsm[0][0]);
    EXPECT_EQ(223, outLinesAsm[0][1]);
    EXPECT_EQ(215, outLinesAsm[0][15]);
    EXPECT_EQ(215, outLinesAsm[1][0]);
    EXPECT_EQ(255, outLinesAsm[1][1]);
    EXPECT_EQ(239, outLinesAsm[1][2]);
    EXPECT_EQ(255, outLinesC[0][0]);
    EXPECT_EQ(223, outLinesC[0][1]);
    EXPECT_EQ(215, outLinesC[0][15]);
    EXPECT_EQ(215, outLinesC[1][0]);
    EXPECT_EQ(255, outLinesC[1][1]);
    EXPECT_EQ(239, outLinesC[1][2]);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 16); 
    outputChecker.CompareArrays(outLinesC[1], outLinesAsm[1], 16);  
    
}