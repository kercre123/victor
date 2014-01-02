//LookupTable test
//  - the function Performs a look-up table transform of an array
//  - the function fills the destination array with values from the look-up table.
//  - indices of the entries are taken from the source array
//
//Asm function prototype:
//    void LUT8to8_asm(u8** src, u8** dest, const u8* lut, u32 width, u32 height);
//
//Asm test function prototype:
//    void LUT8to8_asm_test(u8** src, u8** dest, const u8* lut, u32 width, u32 height);
//    - restrictions: works only for line size multiple of 16
//
//C function prototype:
//    void LUT8to8(u8** src, u8** dest, const u8* lut, u32 width, u32 height)
//
//Parameter description:
//    src - Pointer to source array of elements
//    dest - Pointer to destination array of a given depth and of the same number of channels as the source array
//    lut - Look-up table of 256 elements; should have the same depth as the destination array.
//    width - width of the line from the input array
//    height - the number of lines from the input array

#include "gtest/gtest.h"
#include "LookupTable.h"
#include "LookupTable8to8_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

using namespace mvcv;

class LookupTable8to8Test : public ::testing::Test {
 protected:

    virtual void SetUp()
    {
        randGen.reset(new RandomGenerator);
        uniGen.reset(new UniformGenerator);
        inputGen.AddGenerator(std::string("random"), randGen.get());
        inputGen.AddGenerator(std::string("uniform"), uniGen.get());
    }

    unsigned int width;
    unsigned int height;
    unsigned char** inLines;
    unsigned char* lut;
    unsigned char** outLinesC;
    unsigned char** outLinesAsm;
    ArrayChecker outputChecker;
    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};


TEST_F(LookupTable8to8Test, TestUniformInputLinesAll0)
{
    width = 320;
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width, 1, 0);
    lut = (unsigned char *)inputGen.GetLine(256, 0);
   
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    lut[0] = 20;

    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
    
    for(int j = 0; j < width; j++)
    {
        EXPECT_EQ(20, outLinesC[0][j]);
    }
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(LookupTable8to8Test, TestUniformInputLinesAll255)
{
    width = 320;
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width, 1, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0);
   
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    lut[255] = 30;

    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
    
    for(int j = 0; j < width; j++)
    {
        EXPECT_EQ(30, outLinesC[0][j]);
    }
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(LookupTable8to8Test, TestUniformInputLinesMinimumWidthSize)
{
    width = 8;
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width, 1, 5);
    lut = (unsigned char *)inputGen.GetLine(256, 0);

    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    lut[5] = 19;

    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);

    outputChecker.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 19);
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(LookupTable8to8Test, TestLowestLineSize)
{
    width = 16;
    
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width, 1, 0);
    lut = (unsigned char *)inputGen.GetLine(256, 0);
   
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    
    inLines[0][0] = 18;
    inLines[0][1] = 20;
    inLines[0][2] = 23;
    inLines[0][3] = 7;
    inLines[0][4] = 5;
    inLines[0][5] = 3;
    inLines[0][6] = 75;
    inLines[0][7] = 10;
    inLines[0][8] = 12;
    inLines[0][9] = 15;
    
    lut[18] = 10;
    lut[20] = 20;
    lut[23] = 30;
    lut[7] = 40;
    lut[5] = 50;
    lut[3] = 60;
    lut[75] = 70;
    lut[10] = 80;
    lut[12] = 90;
    lut[15] = 100;
    lut[0] = 200;
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
   
    EXPECT_EQ(10, outLinesC[0][0]);
    EXPECT_EQ(20, outLinesC[0][1]);
    EXPECT_EQ(30, outLinesC[0][2]);
    EXPECT_EQ(40, outLinesC[0][3]);
    EXPECT_EQ(50, outLinesC[0][4]);
    EXPECT_EQ(60, outLinesC[0][5]);
    EXPECT_EQ(70, outLinesC[0][6]);
    EXPECT_EQ(80, outLinesC[0][7]);
    EXPECT_EQ(90, outLinesC[0][8]);
    EXPECT_EQ(100, outLinesC[0][9]);
    EXPECT_EQ(200, outLinesC[0][10]);
    EXPECT_EQ(200, outLinesC[0][11]);
    EXPECT_EQ(200, outLinesC[0][12]);
    EXPECT_EQ(200, outLinesC[0][13]);
    EXPECT_EQ(200, outLinesC[0][14]);
    EXPECT_EQ(200, outLinesC[0][15]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestWidth240RandomData)
{
    width = 24;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestWidth320RandomData)
{
    width = 320;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
      
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestWidth640RandomData)
{
    width = 640;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
      
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestWidth1280RandomData)
{
    width = 1280;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
      
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestWidth1920RandomData)
{
    width = 1920;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
      
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable8to8Test, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = dataGenerator.GenerateUInt(0, 1920, 8);
    inLines = inputGen.GetLines(width, 1, 0, 255);
    lut = (unsigned char *)inputGen.GetLine(256, 0, 255);
    
    inputGen.SelectGenerator("uniform");
    outLinesAsm = inputGen.GetEmptyLines(width, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    
    LUT8to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT8to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTableCycleCount);
      
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}
