//Erode kernel test
//
//Asm function prototype:
//      void Erode(u8** src, u8** dst, u8** kernel, u16 width, u16 height, u8 k)
//
//Asm test function prototype:
//      void Erode_asm_test(u8** src, u8** dst, u8** kernel, u16 width, u16 height, u8 k)
//
//C function prototype:
//      void Erode(u8** src, u8** dst, u8** kernel, u16 width, u16 height, u8 k)
//
//Parameter description:
//  src      - array of pointers to input lines of the input image
//  dst      - array of pointers to output lines
//  kernel   - array of pointers to input kernel
//  width    - width  of the input line
//  height   - height of the input line
//  k        - kernel size

#include "gtest/gtest.h"
#include "imgproc.h"
#include "ErodeGen_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 8

using namespace mvcv;

class ErodeTest : public ::testing::Test {
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
    unsigned char** outLinesC3x3;	
    unsigned char** outLinesC5x5;	
    unsigned char** outLinesC7x7;	
	unsigned char** outLinesAsm;
    unsigned char** inLines;	
    unsigned int width;
    unsigned int k;

	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(ErodeTest, TestUniformInputAll0)
{
	width = 32;
    k = 3;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	Erode(inLines, outLinesC, kernel, width, 1, k);
	Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
        
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues0KernelSize3)
{
	width = 32;
    k = 3;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC3x3 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	Erode3x3(inLines, outLinesC3x3, kernel, width, 1);
	Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
        
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputCheck.CompareArrays(outLinesC3x3[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues0KernelSize5)
{
	width = 32;
    k = 5;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
	Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
        
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues0KernelSize7)
{
	width = 32;
    k = 7;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC7x7 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	Erode7x7(inLines, outLinesC7x7, kernel, width, 1);
	Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
        
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputCheck.CompareArrays(outLinesC7x7[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestUniformInputAll255)
{
	width = 64;
	k = 3;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 255);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode(inLines, outLinesC, kernel, width, 1, k);
	Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
	
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues255KernelSize3)
{
    width = 32;
    k = 3;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 255);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC3x3 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode3x3(inLines, outLinesC3x3, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);

   outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
   outputCheck.CompareArrays(outLinesC3x3[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues255KernelSize5)
{
    width = 32;
    k = 5;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 255);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);

   outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
   outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllValues255KernelSize7)
{
    width = 64;
    k = 7;
    
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 255);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC7x7 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode7x7(inLines, outLinesC7x7, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);

   outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
   outputCheck.CompareArrays(outLinesC7x7[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestLowestLineSizeKernelSize3)
{
    width = 8;
	k = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 2;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 3;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 0; 
     
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
    RecordProperty("CycleCount", ErodeCycleCount);

    EXPECT_EQ(1, outLinesAsm[0][0]);
	EXPECT_EQ(3, outLinesAsm[0][1]);
	EXPECT_EQ(2, outLinesAsm[0][2]);
	EXPECT_EQ(2, outLinesAsm[0][3]);
    EXPECT_EQ(1, outLinesAsm[0][4]);
    EXPECT_EQ(0, outLinesAsm[0][5]);
    EXPECT_EQ(0, outLinesAsm[0][6]);
    EXPECT_EQ(0, outLinesAsm[0][7]);
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestLowestLineSizeKernelSize5)
{
    width = 8;
	k = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 0);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
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
     
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);

    EXPECT_EQ(2, outLinesAsm[0][0]);
	EXPECT_EQ(3, outLinesAsm[0][1]);
	EXPECT_EQ(1, outLinesAsm[0][2]);
	EXPECT_EQ(0, outLinesAsm[0][3]);
    EXPECT_EQ(0, outLinesAsm[0][4]);
    EXPECT_EQ(0, outLinesAsm[0][5]);
    EXPECT_EQ(0, outLinesAsm[0][6]);
    EXPECT_EQ(0, outLinesAsm[0][7]);

    outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestLowestLineSizeKernelSize7)
{
    width = 8;
	k = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 2);
	kernel = inputGen.GetLines(k, k, 1);
	
	outLinesC7x7 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9; 
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
    inLines[3][0] = 8;  inLines[3][1] = 12; inLines[3][2] = 7;  inLines[3][3] = 8; 
    inLines[3][4] = 3;  inLines[3][5] = 25; inLines[3][6] = 19; inLines[3][7] = 4; 
    inLines[4][0] = 5;  inLines[4][1] = 20; inLines[4][2] = 15; inLines[4][3] = 6; 
    inLines[4][4] = 8;  inLines[4][5] = 12; inLines[4][6] = 30; inLines[4][7] = 2; 
    inLines[5][0] = 10; inLines[5][1] = 1;  inLines[5][2] = 5;  inLines[5][3] = 6; 
    inLines[5][4] = 3;  inLines[5][5] = 20; inLines[5][6] = 11; inLines[5][7] = 7; 
    inLines[6][0] = 0;  inLines[6][1] = 6;  inLines[6][2] = 14; inLines[6][3] = 35; 
    inLines[6][4] = 9;  inLines[6][5] = 24; inLines[6][6] = 2;  inLines[6][7] = 18; 
    
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode7x7(inLines, outLinesC7x7, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);

    EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(1, outLinesAsm[0][1]);
	EXPECT_EQ(1, outLinesAsm[0][2]);
	EXPECT_EQ(1, outLinesAsm[0][3]);
    EXPECT_EQ(1, outLinesAsm[0][4]);
    EXPECT_EQ(1, outLinesAsm[0][5]);
    EXPECT_EQ(1, outLinesAsm[0][6]);
    EXPECT_EQ(2, outLinesAsm[0][7]);

    outputCheck.CompareArrays(outLinesC7x7[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestAllKernelValues0)
{
    width = 32;
	k = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 1);
	kernel = inputGen.GetLines(7, 7, 0);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
     
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 3;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 2;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 30; inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 6; 
    
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
    
    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestDiagonalKernel7)
{
    width = 16;
	k = 7;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 5);
	kernel = inputGen.GetLines(k, k, 0);
	
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
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
    
    EXPECT_EQ(0, outLinesAsm[0][0]);
	EXPECT_EQ(3, outLinesAsm[0][1]);
	EXPECT_EQ(1, outLinesAsm[0][2]);
	EXPECT_EQ(3, outLinesAsm[0][3]);
    EXPECT_EQ(4, outLinesAsm[0][4]);
    EXPECT_EQ(0, outLinesAsm[0][5]);
    EXPECT_EQ(2, outLinesAsm[0][6]);
    EXPECT_EQ(5, outLinesAsm[0][7]);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestDiagonalKernel5)
{
    width = 8;
	k = 5;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, k, 2);
	kernel = inputGen.GetLines(k, k, 0);
	
	outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
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
    
    kernel[0][4] = 1;
    kernel[1][3] = 1;
    kernel[2][2] = 1;
    kernel[3][1] = 1;
    kernel[4][0] = 1;
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
    
    EXPECT_EQ(3, outLinesAsm[0][0]);
	EXPECT_EQ(4, outLinesAsm[0][1]);
	EXPECT_EQ(7, outLinesAsm[0][2]);
	EXPECT_EQ(3, outLinesAsm[0][3]);
    EXPECT_EQ(1, outLinesAsm[0][4]);
    EXPECT_EQ(2, outLinesAsm[0][5]);
    EXPECT_EQ(2, outLinesAsm[0][6]);
    EXPECT_EQ(0, outLinesAsm[0][7]);
    
    outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth24RandomData)
{
    width = 24;
	k = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth120RandomData)
{
    width = 120;
	k = 5;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth240RandomData)
{
    width = 240;
	k = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC7x7 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode7x7(inLines, outLinesC7x7, kernel, width, 1);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC7x7[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth320RandomData)
{
    width = 320;
	k = 5;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth640RandomData)
{
    width = 640;
	k = 5;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth1280RandomData)
{
    width = 1280;
	k = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC7x7 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode7x7(inLines, outLinesC7x7, kernel, width, 1);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC7x7[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestWidth1920RandomData)
{
    width = 1920;
	k = 7;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(k, k, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
    RecordProperty("CycleCount", ErodeCycleCount);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestRandomDataRandomKernel)
{
    width = 320;
	k = 3;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
    kernel = inputGen.GetLines(k, k, 0, 2);

   
    inputGen.SelectGenerator("uniform");
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode(inLines, outLinesC, kernel, width, 1, k);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
    
    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(ErodeTest, TestRandomDataRandomKernel2)
{
    width = 320;
	k = 5;
	
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, k, 0, 255);
    kernel = inputGen.GetLines(k, k, 0, 2);

   
    inputGen.SelectGenerator("uniform");
    outLinesC5x5 = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
  
    Erode_asm_test(inLines, outLinesAsm, kernel, width, 1, k);
    Erode5x5(inLines, outLinesC5x5, kernel, width, 1);
	RecordProperty("CyclePerPixel", ErodeCycleCount / width);
    
    outputCheck.CompareArrays(outLinesC5x5[0], outLinesAsm[0], width);
}