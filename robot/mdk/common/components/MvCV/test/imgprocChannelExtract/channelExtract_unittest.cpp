//channelExtract kernel test
//Asm function prototype:
//     void channelExtract_asm(u8** in, u8** out, u32 width, u8 plane);
//
//Asm test function prototype:
//	void channelExtract_asm_test(unsigned char** in, unsigned char** out, unsigned long width, unsigned char plane);
//
//C function prototype:
//     void channelExtract(u8** in, u8** out, u32 width, u8 plane)
//	
//
//Parameter description:
// in        - array of pointers to input lines
// out       - array of pointers for output lines
// plane	 - number 0 to extract plane R, 1 for extracting G, 2 for extracting B
// width     - width of input line
//


#include "gtest/gtest.h"
#include "core.h"
#include "imgproc.h"
#include "channelExtract_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>
#include <stdlib.h>



using namespace mvcv;

class channelExtractKernelTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	unsigned char normalize;
	unsigned char **inLines;
	unsigned char **outLinesC;
	unsigned char **outLinesAsm;
	unsigned int width;
	u8 inputLines[2][1920];		

	InputGenerator inputGen;
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};

//1

TEST_F(channelExtractKernelTest, TestUniformInputLines32)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(30, 1, 1);
	outLinesC = inputGen.GetEmptyLines(10, 1);
	outLinesAsm = inputGen.GetEmptyLines(10, 1);

	channelExtract(inLines, outLinesC, 30, 1);
	channelExtract_asm_test(inLines, outLinesAsm, 30, 1);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / 30);

	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], 10, 1);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], 10);
}

//2			TEST G

TEST_F(channelExtractKernelTest, TestGreenChannelExtract)
{
	width = 330;
	
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 34);
		
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);
	
	for(int i=0; i<=width; i++)
	{
		if(i % 3 == 1) {
			inLines[0][i] = 35;
		}
		if(i % 3 == 2) {
			inLines[0][i] = 36;
		}
	}
	
	channelExtract(inLines,outLinesC,width,1);
	channelExtract_asm_test(inLines, outLinesAsm, width, 1);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);
	
	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width / 3, 35);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}

//3			TEST R

TEST_F(channelExtractKernelTest, TestRedChannelExtract)
{
	width = 330;
			
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 34);
				
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);
			
	for(int i=0; i<=width; i++)
	{
		if(i % 3 == 1) 
		{
			inLines[0][i] = 35;
		}
		if(i % 3 == 2) 
		{
			inLines[0][i] = 36;
		}
	}
			
	channelExtract(inLines,outLinesC,width,0);
	channelExtract_asm_test(inLines, outLinesAsm, width, 0);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);
			
	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width / 3, 34);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}
	
// 4		TEST B

TEST_F(channelExtractKernelTest, TestBlueChannelExtract)    
{
	width = 330;
			
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, 1, 34);
				
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);
			
	for(int i=0; i<=width; i++)
	{
		if(i % 3 == 1) 
			inLines[0][i] = 35;
		
		if(i % 3 == 2) 
			inLines[0][i] = 36;
		
	}
			
	channelExtract(inLines,outLinesC,width,2);
	channelExtract_asm_test(inLines, outLinesAsm, width, 2);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);
			
	outputChecker.ExpectAllEQ<unsigned char>(outLinesC[0], width / 3, 36);
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}

//5  TEST RANDOM RED

TEST_F(channelExtractKernelTest, TestRandomDataRed)
{
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);

	channelExtract(inLines, outLinesC, width, 0);
	channelExtract_asm_test(inLines, outLinesAsm, width,0);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}

//6  TEST RANDOM GREEN

TEST_F(channelExtractKernelTest, TestRandomDataGreen)
{
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);

	channelExtract(inLines, outLinesC, width, 1);
	channelExtract_asm_test(inLines, outLinesAsm, width,1);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}

//7  TEST RANDOM BLUE

TEST_F(channelExtractKernelTest, TestRandomDataBlue)
{
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	outLinesC = inputGen.GetEmptyLines(width / 3, 2);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 2);

	channelExtract(inLines, outLinesC, width, 2);
	channelExtract_asm_test(inLines, outLinesAsm, width,2);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
}


//8 TEST RANDOM POSITION RANDOM DATA RED

TEST_F(channelExtractKernelTest, TestRandomPositionRandomDataRed)
{
	
	int r = rand() %110+1;
	
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	inLines[0][0]=20;
	inLines[0][330]=100;
	
	outLinesC = inputGen.GetEmptyLines(width , 2);
	outLinesAsm = inputGen.GetEmptyLines(width , 2);

	channelExtract(inLines, outLinesC, width, 0);
	channelExtract_asm_test(inLines, outLinesAsm, width,0);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
	EXPECT_EQ(outLinesC[0][r], outLinesAsm[0][r]);
	

}

//9 TEST RANDOM POSITION RANDOM DATA GREEN

TEST_F(channelExtractKernelTest, TestRandomPositionRandomDataGreen)
{
	
	int r = rand() %110+1;
	
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	inLines[0][0]=20;
	inLines[0][330]=100;
	
	outLinesC = inputGen.GetEmptyLines(width , 2);
	outLinesAsm = inputGen.GetEmptyLines(width , 2);

	channelExtract(inLines, outLinesC, width, 1);
	channelExtract_asm_test(inLines, outLinesAsm, width,1);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
	EXPECT_EQ(outLinesC[0][r], outLinesAsm[0][r]);
	

}


//10 TEST RANDOM POSITION RANDOM DATA BLUE

TEST_F(channelExtractKernelTest, TestRandomPositionRandomDataBlue)
{
	
	int r = rand() %110+1;
	
	width = 330;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, 2, 0, 255);
	
	inLines[0][0]=20;
	inLines[0][330]=100;
	
	outLinesC = inputGen.GetEmptyLines(width , 2);
	outLinesAsm = inputGen.GetEmptyLines(width , 2);

	channelExtract(inLines, outLinesC, width, 2);
	channelExtract_asm_test(inLines, outLinesAsm, width,2);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
	EXPECT_EQ(outLinesC[0][r], outLinesAsm[0][r]);
	

}

//11 TEST EDGES

TEST_F(channelExtractKernelTest, TestEdges)
{
	width = 330;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(330, 1, 1);
	for(int i=0; i<3;i++)
		inLines[0][i]=20;
	for(int j=329; j>=227;j--)
		inLines[0][j]=100;
	
	outLinesC = inputGen.GetEmptyLines(width / 3, 1);
	outLinesAsm = inputGen.GetEmptyLines(width / 3, 1);

	channelExtract(inLines, outLinesC, width, 2);
	channelExtract_asm_test(inLines, outLinesAsm, width,2);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);	
	
	outputChecker.CompareArrays(outLinesC[0], outLinesAsm[0], width / 3);
	EXPECT_EQ(outLinesC[0][1], outLinesAsm[0][1]);
	EXPECT_EQ(outLinesC[0][109], outLinesAsm[0][109]);
	
	
}

//12

TEST_F(channelExtractKernelTest, TestMinimumWidthSize)
{
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(3 , 1, 1);
	
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

	channelExtract(inLines, outLinesC, width, 2);
	channelExtract_asm_test(inLines, outLinesAsm, width, 2);
	RecordProperty("CyclePerPixel", channelExtractCycleCount / width);

	EXPECT_EQ(outLinesC[0][1], outLinesAsm[0][1]);
	
	
	
}
