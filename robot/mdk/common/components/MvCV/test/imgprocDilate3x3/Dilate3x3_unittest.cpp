//Dilate3x3 test
//Asm function prototype:
//     void Dilate3x3_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//
//Asm test function prototype:
//     void Dilate3x3_asm_test(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//
//C function prototype:
//     void Dilate3x3(u8** src, u8** dst, u8** kernel, u16 width, u16 height);
//
//Parameter description:
//  src      - array of pointers to input lines of the input image
//  dst      - array of pointers to output lines
//  kernel   - array of pointers to input kernel
//  width    - width  of the input line
//  height   - height of the input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "Dilate3x3_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

#define PADDING 4

using namespace mvcv;

class Dilate3x3Test : public ::testing::Test {
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
    unsigned char** kernel;
    unsigned char** outLinesC;
    unsigned char** outLinesAsm;
    ArrayChecker outputChecker;
    RandomGenerator dataGenerator;
    InputGenerator inputGen;
	ArrayChecker outputCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};

TEST_F(Dilate3x3Test, TestAllValues0)
{
    width = 64;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);

    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestAllValues255)
{
    width = 64;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	kernel = inputGen.GetLines(3, 3, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);

    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate3x3Test, TestLowestLineSize)
{
    width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
     
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);

    EXPECT_EQ(30, outLinesC[0][0]);
    EXPECT_EQ(30, outLinesC[0][1]);
    EXPECT_EQ(30, outLinesC[0][2]);
    EXPECT_EQ(15, outLinesC[0][3]);
    EXPECT_EQ(15, outLinesC[0][4]);
    EXPECT_EQ(12, outLinesC[0][5]);
    EXPECT_EQ(12, outLinesC[0][6]);
    EXPECT_EQ(9, outLinesC[0][7]);

    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestAllKernelValues0)
{
    width = 32;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 0);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
    
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestDiagonalKernel)
{
    width = 64;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 0);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
   
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
    
    kernel[0][2] = 1;
    kernel[1][1] = 1;
    kernel[2][0] = 1;
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    EXPECT_EQ(20, outLinesAsm[0][0]);
	EXPECT_EQ(9, outLinesAsm[0][1]);
	EXPECT_EQ(30, outLinesAsm[0][2]);
	EXPECT_EQ(15, outLinesAsm[0][3]);
    EXPECT_EQ(12, outLinesAsm[0][4]);
    EXPECT_EQ(9, outLinesAsm[0][5]);
    EXPECT_EQ(9, outLinesAsm[0][6]);
    EXPECT_EQ(6, outLinesAsm[0][7]);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate3x3Test, TestWidth24RandomData)
{
    width = 24;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate3x3Test, TestWidth120RandomData)
{
    width = 120;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestWidth640RandomData)
{
    width = 640;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestWidth1280RandomData)
{
    width = 1280;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestWidth1920RandomData)
{
    width = 1920;//1912;//1926;//1912;//1904;//1928;//1920;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate3x3Test, TestRandomDataRandomKernel)
{
    width = 240;
	height = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    kernel = inputGen.GetLines(3, 3, 0, 2);

   
    inputGen.SelectGenerator("uniform");
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate3x3_asm_test(inLines, outLinesAsm, kernel, width, 1);
    Dilate3x3(inLines, outLinesC, kernel, width, 1);
    RecordProperty("CycleCount", Dilate3x3CycleCount);
    
    outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}