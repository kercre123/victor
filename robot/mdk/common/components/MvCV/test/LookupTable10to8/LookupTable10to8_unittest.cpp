//LookupTable test
//  - the function Performs a look-up table transform of an array
//  - the function fills the destination array with values from the look-up table.
//  - indices of the entries are taken from the source array
//
//Asm function prototype:
//    LUT10to8_asm(u16** src, u8** dest, const u16* lut, u32 width, u32 height);
//
//Asm test function prototype:
//    void LUT10to8_asm_test(u16** src, u8** dest, const u16* lut, u32 width, u32 height);
//    - restrictions: works only for line size multiple of 16
//
//C function prototype:
//    void LUT10to8(u16** src, u8** dest, const u16* lut, u32 width, u32 height);
//
//Parameter description:
//    src - Pointer to source array of elements
//    dest - Pointer to destination array of a given depth and of the same number of channels as the source array
//    lut - Look-up table of 256 elements; should have the same depth as the destination array.
//    width - width of the line from the input array
//    height - the number of lines from the input array

#include "gtest/gtest.h"
#include "LookupTable.h"
#include "LookupTable10to8_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

using namespace mvcv;

class LookupTable10to8Test : public ::testing::Test {
 protected:

    virtual void SetUp()
    {
        randGen.reset(new RandomGenerator);
        uniGen.reset(new UniformGenerator);
        inputGen.AddGenerator(std::string("random"), randGen.get());
        inputGen.AddGenerator(std::string("uniform"), uniGen.get());
    }

    u32 width;
    u32 height;
    u16** inLines;
    u8* lut;
    u8** outLinesC;
    u8** outLinesAsm;
    ArrayChecker outputChecker;
    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};

TEST_F(LookupTable10to8Test, TestUniformInputLinesAll0)
{
    width = 320;
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetLines(width * 2, 1, 0);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
 
    lut[0] = 100;

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
    
    for(int j = 0; j < width; j++)
    {
        EXPECT_EQ(100, outLinesC[0][j]);
    }
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(LookupTable10to8Test, TestUniformInputLinesAll255)
{
    width = 32;
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = 255;
    }
 
    lut[255] = 50;

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
    
    for(int j = 0; j < width; j++)
    {
        EXPECT_EQ(50, outLinesC[0][j]);
    }
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(LookupTable10to8Test, TestLowestLineSize)
{
    width = 16;
    
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetLines(width * 2, 1, 0);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
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
    
    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
   
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


TEST_F(LookupTable10to8Test, TestOnlyOneMaskedValue)
{
    width = 32;
    
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetLines(width * 2, 1, 0);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inLines[0][3] = 5000;
    
    lut[904] = 10;
    
    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
   
    EXPECT_EQ(10, outLinesC[0][3]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(LookupTable10to8Test, TestMaskedValues)
{
    width = 32;
    
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetLines(width * 2, 1, 0);
    lut = (u8*)inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);

    inLines[0][0] = 4098;
    inLines[0][1] = 5020;
    inLines[0][2] = 6000;
    inLines[0][3] = 6500;
    inLines[0][31] = 65535;
    
    lut[2] = 40;
    lut[924] = 50;
    lut[880] = 60;
    lut[356] = 70;
    lut[1023] = 80;
    
    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);

    EXPECT_EQ(40, outLinesC[0][0]);
    EXPECT_EQ(50, outLinesC[0][1]);
    EXPECT_EQ(60, outLinesC[0][2]);
    EXPECT_EQ(70, outLinesC[0][3]);
    EXPECT_EQ(80, outLinesC[0][31]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(LookupTable10to8Test, TestWidth240RandomData)
{
    width = 240;

    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable10to8Test, TestWidth320RandomData)
{
    width = 320;

    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable10to8Test, TestWidth640RandomData)
{
    width = 640;

    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable10to8Test, TestWidth1280RandomData)
{
    width = 1280;

    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable10to8Test, TestWidth1920RandomData)
{
    width = 1920;

    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetEmptyLines(width * 2, 1);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(LookupTable10to8Test, TestRandomWidthRandomData)
{
    inputGen.SelectGenerator("random");
    width = dataGenerator.GenerateUInt(0, 1920);
    inputGen.SelectGenerator("uniform");
    inLines = (u16**)inputGen.GetLines(width * 2, 1, 0);
    lut = inputGen.GetLine(4096, 0);
    outLinesC = (u8**)inputGen.GetEmptyLines(width, 1);
    outLinesAsm = (u8**)inputGen.GetEmptyLines(width, 1);
    
    inputGen.SelectGenerator("random");
    for(int j = 0; j < width; j++)
    {
        inLines[0][j] = dataGenerator.GenerateUInt(0, 255);
    }

    LUT10to8_asm_test(inLines, outLinesAsm, lut, width, 1);
    LUT10to8(inLines, outLinesC, lut, width, 1);
    RecordProperty("CycleCount", LookupTable10to8CycleCount);
     
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);


}
