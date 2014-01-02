//HistogramTest
//Asm function prototype:
//   void histogram_asm_test(u8** in, u32 *hist, u32 width);
//
//Asm test function prototype:
//   void histogram_asm_test(u8** in, u32 *hist, u32 width);
//
//C function prototype:
//   void histogram(u8** in, u32 *hist, u32 width);
//
//Parameter description:
// in        - array of pointers to input lines
// hist      - array oh values from histogram
// width     - width of input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "core.h"
#include "histogram_asm_test.h"
#include "InputGenerator.h"
#include "RandomGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;


class HistogramTest : public ::testing::Test {
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
    long unsigned int *asmHist;
    long unsigned int *cHist;
    ArrayChecker outputChecker;
    
    virtual void TearDown()
    {
    }
};

TEST_F(HistogramTest, TestAsmAllValues0)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram_asm_test(inLines, asmHist, 320);

    RecordProperty("CycleCount", histogramCycleCount);
       
    EXPECT_EQ(320, cHist[0]);
    EXPECT_EQ(320, asmHist[0]);
    EXPECT_EQ(0, cHist[319]);
    EXPECT_EQ(0, asmHist[319]);
        
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestAsmAllValues255)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    for( int j = 0; j < 320; j++)
    {
        inLines[0][j] = 255;
    }
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram_asm_test(inLines, asmHist, 320);

    RecordProperty("CycleCount", histogramCycleCount);
       
    EXPECT_EQ(320, cHist[255]);
    EXPECT_EQ(320, asmHist[255]);
    EXPECT_EQ(0, cHist[10]);
    EXPECT_EQ(0, asmHist[50]);
        
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestAsmAllValuesOne)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(320, 1, 0);
    
    for( int j = 0; j < 320; j++)
    {
        inLines[0][j] = 1;
    }
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 320);
    histogram_asm_test(inLines, asmHist, 320);

    RecordProperty("CycleCount", histogramCycleCount);
       
    EXPECT_EQ(320, cHist[1]);
    EXPECT_EQ(320, asmHist[1]);
    EXPECT_EQ(0, cHist[255]);
    EXPECT_EQ(0, asmHist[0]);
        
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestAsmLowestLineSize)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 1, 0);
    
    inLines[0][0] = 0;
    inLines[0][1] = 18;
    inLines[0][2] = 3;
    inLines[0][3] = 25;
    inLines[0][4] = 78;
    inLines[0][5] = 100;
    inLines[0][6] = 10;
    inLines[0][7] = 2;
    inLines[0][8] = 1;
    inLines[0][9] = 2;
    inLines[0][10] = 200;
    inLines[0][11] = 5;
    inLines[0][12] = 5;
    inLines[0][13] = 2;
    inLines[0][14] = 120;
    inLines[0][15] = 119;
    
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, 16);
    histogram_asm_test(inLines, asmHist, 16);

    RecordProperty("CycleCount", histogramCycleCount);

    EXPECT_EQ(1, cHist[0]);
    EXPECT_EQ(1, cHist[1]);
    EXPECT_EQ(3, cHist[2]);
    EXPECT_EQ(1, cHist[3]);
    EXPECT_EQ(0, cHist[4]);
    EXPECT_EQ(2, cHist[5]);
    EXPECT_EQ(0, cHist[6]);
    EXPECT_EQ(1, cHist[10]);
    EXPECT_EQ(1, cHist[25]);
    EXPECT_EQ(1, cHist[78]);
    EXPECT_EQ(1, cHist[10]);
    EXPECT_EQ(1, cHist[120]);
    EXPECT_EQ(1, cHist[200]);

    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth120RandomData)
{
    width = 120;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
    
    RecordProperty("CycleCount", histogramCycleCount);
    
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth240RandomData)
{
    width = 240;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
    
    RecordProperty("CycleCount", histogramCycleCount);
    
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth320RandomData)
{
    width = 320;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
    
    RecordProperty("CycleCount", histogramCycleCount);
    
    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth640RandomData)
{
    width = 640;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
  
    RecordProperty("CycleCount", histogramCycleCount);

    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth1280RandomData)
{
    width = 1280;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
  
    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);

    RecordProperty("CycleCount", histogramCycleCount);

    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestWidth1920RandomData)
{
    width = 1920;
  
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
    
    RecordProperty("CycleCount", histogramCycleCount);

    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = ranGen.GenerateUInt(0, 1920);
    inLines = inputGen.GetLines(width, 1, 0, 255);

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(inLines, cHist, width);
    histogram_asm_test(inLines, asmHist, width);
    
    RecordProperty("CycleCount", histogramCycleCount);

    outputChecker.CompareArrays(cHist, asmHist, 256);
}

TEST_F(HistogramTest, TestAsmTwoLines)
{
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(16, 2, 0);
  
    inLines[0][0] = 5;
    inLines[0][1] = 2;
    inLines[0][2] = 120;
    inLines[1][1] = 5;
    inLines[1][2] = 255;

    inputGen.SelectGenerator("uniform");
    asmHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);
    cHist = (long unsigned int *)inputGen.GetLine(256 * 4, 0);

    histogram(&inLines[0], cHist, 16);
    histogram_asm_test(&inLines[0], asmHist, 16);
    histogram(&inLines[1], cHist, 16);
    histogram_asm_test(&inLines[1], asmHist, 16);
    
    RecordProperty("CycleCount", histogramCycleCount);
 
    EXPECT_EQ(27, cHist[0]);
    EXPECT_EQ(0, cHist[1]);
    EXPECT_EQ(1, cHist[2]);
    EXPECT_EQ(2, cHist[5]);
    EXPECT_EQ(1, cHist[120]);
    
    outputChecker.CompareArrays(cHist, asmHist, 256);
}
