//Dilate7x7 test
//Asm function prototype:
//     void Dilate7x7_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//
//Asm test function prototype:
//     void Dilate7x7_asm_test(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//
//C function prototype:
//     void Dilate7x7(u8** src, u8** dst, u8** kernel, u16 width, u16 height);
//
//Parameter description:
//  src      - array of pointers to input lines of the input image
//  dst      - array of pointers to output lines
//  kernel   - array of pointers to input kernel
//  width    - width  of the input line
//  height   - height of the input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "Dilate7x7_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 8

using namespace mvcv;

class Dilate7x7Test : public ::testing::Test {
 protected:

    virtual void SetUp()
    {
        randGen.reset(new RandomGenerator);
        uniGen.reset(new UniformGenerator);
        inputGen.AddGenerator(std::string("random"), randGen.get());
        inputGen.AddGenerator(std::string("uniform"), uniGen.get());
    }

    unsigned char** kernel;		
    unsigned char** outLinesC;	
	unsigned char** outLinesAsm;
    unsigned char** inLines;	
    unsigned int height;
    unsigned int width;

	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(Dilate7x7Test, TestAllValues0)
{
    width = 64;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(7, 7, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);

    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestAllValues255)
{
    width = 64;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	kernel = inputGen.GetLines(7, 7, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);

    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate7x7Test, TestLowestLineSize)
{
    width = 8;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(7, 7, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    
    inLines[0][0] = 7;  inLines[0][1] = 13;  inLines[0][2] = 20;  inLines[0][3] = 9; 
    inLines[0][4] = 3;  inLines[0][5] = 8;   inLines[0][6] = 12;  inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10;  inLines[1][2] = 6;   inLines[1][3] = 11;
    inLines[1][4] = 15; inLines[1][5] = 10;  inLines[1][6] = 9;   inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;   inLines[2][2] = 30;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;   inLines[2][6] = 1;   inLines[2][7] = 6; 
    inLines[3][0] = 8;  inLines[3][1] = 12;  inLines[3][2] = 7;   inLines[3][3] = 8; 
    inLines[3][4] = 3;  inLines[3][5] = 25;  inLines[3][6] = 19;  inLines[3][7] = 4; 
    inLines[4][0] = 5;  inLines[4][1] = 20;  inLines[4][2] = 15;  inLines[4][3] = 6; 
    inLines[4][4] = 8;  inLines[4][5] = 12;  inLines[4][6] = 10;  inLines[4][7] = 0;
    inLines[5][0] = 10; inLines[5][1] = 1;   inLines[5][2] = 5;   inLines[5][3] = 6; 
    inLines[5][4] = 3;  inLines[5][5] = 20;  inLines[5][6] = 11;  inLines[5][7] = 7; 
    inLines[6][0] = 0;  inLines[6][1] = 6;   inLines[6][2] = 14;  inLines[6][3] = 35;
    inLines[6][4] = 9;  inLines[6][5] = 24;  inLines[6][6] = 2;   inLines[6][7] = 18;
    
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);

    EXPECT_EQ(35, outLinesC[0][0]);
    EXPECT_EQ(35, outLinesC[0][1]);
    EXPECT_EQ(35, outLinesC[0][2]);
    EXPECT_EQ(35, outLinesC[0][3]);
    EXPECT_EQ(25, outLinesC[0][4]);
    EXPECT_EQ(25, outLinesC[0][5]);
    EXPECT_EQ(19, outLinesC[0][6]);
    EXPECT_EQ(18, outLinesC[0][7]);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestAllKernelValues0)
{
    width = 32;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 1);
	kernel = inputGen.GetLines(7, 7, 0);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
    
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestDiagonalKernel)
{
    width = 16;
	height = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 2);
	kernel = inputGen.GetLines(7, 7, 0);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
   
    inLines[0][0] = 7;  inLines[0][1] = 13;  inLines[0][2] = 20;  inLines[0][3] = 9; 
    inLines[0][4] = 3;  inLines[0][5] = 8;   inLines[0][6] = 12;  inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10;  inLines[1][2] = 6;   inLines[1][3] = 11;
    inLines[1][4] = 15; inLines[1][5] = 10;  inLines[1][6] = 9;   inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;   inLines[2][2] = 30;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;   inLines[2][6] = 1;   inLines[2][7] = 6; 
    inLines[3][0] = 8;  inLines[3][1] = 12;  inLines[3][2] = 7;   inLines[3][3] = 8; 
    inLines[3][4] = 3;  inLines[3][5] = 25;  inLines[3][6] = 19;  inLines[3][7] = 4; 
    inLines[4][0] = 5;  inLines[4][1] = 20;  inLines[4][2] = 15;  inLines[4][3] = 6; 
    inLines[4][4] = 8;  inLines[4][5] = 12;  inLines[4][6] = 30;  inLines[4][7] = 0;
    inLines[5][0] = 10; inLines[5][1] = 1;   inLines[5][2] = 5;   inLines[5][3] = 6; 
    inLines[5][4] = 3;  inLines[5][5] = 20;  inLines[5][6] = 11;  inLines[5][7] = 7; 
    inLines[6][0] = 0;  inLines[6][1] = 6;   inLines[6][2] = 14;  inLines[6][3] = 35; 
    inLines[6][4] = 9;  inLines[6][5] = 24;  inLines[6][6] = 2;   inLines[6][7] = 18; 
    
    kernel[0][6] = 1;
    kernel[1][5] = 1;
    kernel[2][4] = 1;
    kernel[3][3] = 1;
    kernel[4][2] = 1;
    kernel[5][1] = 1;
    kernel[6][0] = 1;
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    EXPECT_EQ(15, outLinesAsm[0][0]);
	EXPECT_EQ(9, outLinesAsm[0][1]);
	EXPECT_EQ(25, outLinesAsm[0][2]);
	EXPECT_EQ(35, outLinesAsm[0][3]);
    EXPECT_EQ(30, outLinesAsm[0][4]);
    EXPECT_EQ(24, outLinesAsm[0][5]);
    EXPECT_EQ(7, outLinesAsm[0][6]);
    EXPECT_EQ(18, outLinesAsm[0][7]);
    EXPECT_EQ(2, outLinesAsm[0][8]);
    EXPECT_EQ(2, outLinesAsm[0][9]);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate7x7Test, TestWidth24RandomData)
{
    width = 24;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(7, 7, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Dilate7x7Test, TestWidth120RandomData)
{
    width = 120;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(7, 7, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestWidth640RandomData)
{
    width = 640;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(7, 7, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestWidth1280RandomData)
{
    width = 1280;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(7, 7, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestWidth1920RandomData)
{
    width = 1920;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(7, 7, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}

TEST_F(Dilate7x7Test, TestRandomDataRandomKernel)
{
    width = 240;
	height = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    kernel = inputGen.GetLines(7, 7, 0, 2);

   
    inputGen.SelectGenerator("uniform");
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Dilate7x7_asm_test(inLines, outLinesAsm, kernel, width, height);
    Dilate7x7(inLines, outLinesC, kernel, width, height);
	RecordProperty("CyclePerPixel", Dilate7x7CycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);

}