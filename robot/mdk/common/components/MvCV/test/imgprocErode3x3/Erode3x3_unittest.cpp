//Erode3x3 kernel test

//Asm function prototype:
//      void Erode3x3(u8** src, u8** dst, u8** kernel, u16 width, u16 height)

//Asm test function prototype:
//      void Erode3x3_asm_test(u8** src, u8** dst, u8** kernel, u16 width, u16 height)

//C function prototype:
//      void Erode3x3(u8** src, u8** dst, u8** kernel, u16 width, u16 height)

// src      - array of pointers to input lines of the input image
// dst      - array of pointers to output lines
// kernel   - array of pointers to input kernel
// width    - width  of the input line
// height   - height of the input line

#include "gtest/gtest.h"
#include "imgproc.h"
#include "Erode3x3_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 4

using namespace mvcv;

class Erode3x3Test : public ::testing::Test {
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
    unsigned char** inLinesC;  
    unsigned char** inLines;
	unsigned int width;
    unsigned int height;
   
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(Erode3x3Test, TestUniformInputAll0)
{
	width = 64;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
	
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
	
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 0);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestUniformInputAll255)
{
	width = 64;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 255);
	kernel = inputGen.GetLines(3, 3, 1);
	
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
	
	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestNonUniformInput)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 1);
	
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 2;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 3;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 0; 
    
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);

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

TEST_F(Erode3x3Test, TestAllKernelValues0)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 0);

    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9;
    inLines[0][4] = 2;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11;
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 3;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 0; 
    
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
 	outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 255);
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestDiagonalKernel)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 0);
	
    kernel[0][2] = 1;
    kernel[1][1] = 1;
    kernel[2][0] = 1;
    
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9; 
    inLines[0][4] = 2;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11;
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 3;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 0; 
    
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);

    EXPECT_EQ(9, outLinesAsm[0][0]);
	EXPECT_EQ(6, outLinesAsm[0][1]);
	EXPECT_EQ(2, outLinesAsm[0][2]);
	EXPECT_EQ(4, outLinesAsm[0][3]);
    EXPECT_EQ(7, outLinesAsm[0][4]);
    EXPECT_EQ(3, outLinesAsm[0][5]);
    EXPECT_EQ(0, outLinesAsm[0][6]);
    EXPECT_EQ(0, outLinesAsm[0][7]);
 
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestOtherKernelForm)
{
	width = 8;
	height = 3;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, height, 0);
	kernel = inputGen.GetLines(3, 3, 0);
	
    kernel[1][1] = 1;
    kernel[2][0] = 1;
    kernel[2][1] = 1;
    kernel[2][2] = 1;
    
    inLines[0][0] = 7;  inLines[0][1] = 13; inLines[0][2] = 20; inLines[0][3] = 9; 
    inLines[0][4] = 2;  inLines[0][5] = 8;  inLines[0][6] = 12; inLines[0][7] = 3; 
	inLines[1][0] = 1;  inLines[1][1] = 10; inLines[1][2] = 6;  inLines[1][3] = 11; 
    inLines[1][4] = 15; inLines[1][5] = 10; inLines[1][6] = 9;  inLines[1][7] = 9; 
	inLines[2][0] = 9;  inLines[2][1] = 8;  inLines[2][2] = 3;  inLines[2][3] = 4; 
    inLines[2][4] = 7;  inLines[2][5] = 4;  inLines[2][6] = 1;  inLines[2][7] = 0; 
    
	outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
    EXPECT_EQ(3, outLinesAsm[0][0]);
	EXPECT_EQ(3, outLinesAsm[0][1]);
	EXPECT_EQ(3, outLinesAsm[0][2]);
	EXPECT_EQ(4, outLinesAsm[0][3]);
    EXPECT_EQ(1, outLinesAsm[0][4]);
    EXPECT_EQ(0, outLinesAsm[0][5]);
    EXPECT_EQ(0, outLinesAsm[0][6]);
    EXPECT_EQ(0, outLinesAsm[0][7]);
 
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestWidth24RandomData)
{
	width = 24;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestWidth120RandomData)
{
	width = 120;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestWidth640RandomData)
{
	width = 640;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestWidth1280RandomData)
{
	width = 1280;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestWidth1920RandomData)
{
	width = 1920;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   
    inputGen.SelectGenerator("uniform");
	kernel = inputGen.GetLines(3, 3, 1);
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(Erode3x3Test, TestRandomDataRandomKernel)
{
	width = 240;
	height = 3;
   
    inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
   	kernel = inputGen.GetLines(3, 3, 0, 2);

    inputGen.SelectGenerator("uniform");
    outLinesC = inputGen.GetEmptyLines(width , 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
		
	Erode3x3(inLines, outLinesC, kernel, width, height);
	Erode3x3_asm_test(inLines, outLinesAsm, kernel, width, height);
	RecordProperty("CyclePerPixel", Erode3x3CycleCount / width);
    
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}