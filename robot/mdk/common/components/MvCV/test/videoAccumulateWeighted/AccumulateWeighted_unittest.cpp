//AccumulateWeighted kernel test

//Asm function prototype:
//	void AccumulateWeighted_asm(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, fp32 alpha);

//Asm test function prototype:
//    	void AccumulateWeighted_asm_test(unsigned char** srcAddr, unsigned char** maskAddr, float** destAddr, unsigned int width, float alpha);

//C function prototype:
//  	void AccumulateWeighted(u8** srcAddr, u8** maskAddr, fp32** destAddr, u32 width, fp32 alpha);

//srcAddr	 - array of pointers to input lines      
//maskAddr  - array of pointers to input lines of mask      
//destAddr	 - array of pointers for output lines    
//width  	 - width of input line
//alpha	 - Weight of the input image must be a fp32 between 0 and 1

#include "gtest/gtest.h"
#include "video.h"
#include "AccumulateWeighted_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;

class AccumulateWeightedTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
		noLines = 1;
	}	
	
	unsigned char **inLines;
	float **outLinesC;
	float **outLinesAsm;
	float **aux;
	unsigned char **inMask;
	unsigned int width;
	float alpha;	
	unsigned int noLines;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

TEST_F(AccumulateWeightedTest, TestUniformInput0Mask0Alpha0)
{
	width = 320;
	alpha = 0;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, noLines, 0);
	inMask = inputGen.GetLines(width, noLines, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	aux = (float**) inputGen.GetEmptyLines(width * sizeof(float), noLines);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	outputCheck.CompareArrays(aux[0], outLinesC[0], width);
	outputCheck.CompareArrays(aux[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestUniformInputMinWidth)
{
	width = 8;
	alpha = 1.0;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, noLines, 7);
	inMask = inputGen.GetLines(width, noLines, 1);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(outLinesC, width, noLines, 3.0);

	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


TEST_F(AccumulateWeightedTest, TestRandomInputMask0Alpha0345)
{
	width = 320;
	alpha = 0.345;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);	
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestUniformInput55Mask1Alpha0Dest255)
{
	width = 640;
	alpha = 0;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, noLines, 55);
	inMask = inputGen.GetLines(width, noLines, 1);
	aux = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(aux, width, noLines, 255);
	inputGen.FillLines(outLinesC, width, noLines, 255);
	inputGen.FillLines(outLinesAsm, width, noLines, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestUniformInput255Mask1Alpha1)
{
	width = 640;
	alpha = 1;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, noLines, 255);
	inMask = inputGen.GetLines(width, noLines, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	aux = (float**)inputGen.GetEmptyLines(width * sizeof(float), noLines);
	inputGen.FillLines(aux, width, noLines, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask1Alpha1Dest255)
{
	width = 640;
	alpha = 1;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	inputGen.FillLines(outLinesC, width, noLines, 255);
	inputGen.FillLines(outLinesAsm, width, noLines, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(inLines[0][i], outLinesC[0][i]);
		EXPECT_FLOAT_EQ(inLines[0][i], outLinesAsm[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask1Alpha05Dest255)
{
	width = 160;
	alpha = 0.5;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(outLinesC, width, noLines, 255);
	inputGen.FillLines(outLinesAsm, width, noLines, 255);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(((inLines[0][i] * alpha) + ((1 - alpha) * 255)), outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask1Alpha0123Dest75MinWidth)
{
	width = 32;
	alpha = 0.123;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 1);
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(outLinesC, width, noLines, 75);
	inputGen.FillLines(outLinesAsm, width, noLines, 75);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(((inLines[0][i] * alpha) + ((1 - alpha) * 75)), outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask10Alpha07Dest200)
{
	width = 800;
	alpha = 0.7;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 0);
	for(int i = 0; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(outLinesC, width, noLines, 200);
	inputGen.FillLines(outLinesAsm, width, noLines, 200);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(((inLines[0][i] * alpha) + ((1 - alpha) * 200)), outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
		i++;
		EXPECT_FLOAT_EQ(200, outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask01Alpha07Dest147MaxWidth)
{
	width = 1920;
	alpha = 0.7;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 0);
	for(int i = 1; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}
	outLinesC = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	inputGen.FillLines(outLinesC, width, noLines, 147);
	inputGen.FillLines(outLinesAsm, width, noLines, 147);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(147, outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
		i++;
		EXPECT_FLOAT_EQ(((inLines[0][i] * alpha) + ((1 - alpha) * 147)), outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputMask01Alpha0753DestRandomMaxWidth)
{
	width = 32;
	alpha = 0.753;
	inputGen.SelectGenerator("uniform");
	inMask = inputGen.GetLines(width, noLines, 0);
	outLinesAsm = (float**)inputGen.GetLines(width * sizeof(float), noLines, 0);
	aux = (float**)inputGen.GetEmptyLines(width * sizeof(float), noLines);
	for(int i = 1; i < width; i = i + 2)
	{
		inMask[0][i] = 1;
	}	
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	outLinesC = inputGen.GetLinesFloat(width * sizeof(float), noLines, 0, 255);
	for(int i = 0; i < width; i++)
	{
		aux[0][i] = outLinesC[0][i];
		outLinesAsm[0][i] = outLinesC[0][i];
	}
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(aux[0][i], outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
		i++;
		EXPECT_FLOAT_EQ(((inLines[0][i] * alpha) + ((1 - alpha) * aux[0][i])), outLinesC[0][i]);
		EXPECT_FLOAT_EQ(outLinesAsm[0][i], outLinesC[0][i]);
	}
}

TEST_F(AccumulateWeightedTest, TestRandomInputRandomMaskAlpha012941)
{
	width = 480;
	alpha = 0.12941;
	inputGen.SelectGenerator("uniform");
	outLinesC = inputGen.GetLinesFloat(width * sizeof(float), noLines, 0);
	outLinesAsm = inputGen.GetLinesFloat(width * sizeof(float), noLines, 0);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	inMask = inputGen.GetLines(width, noLines, 0, 2);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestRandomInputRandomMaskAlpha05Dest99)
{
	width = 320;
	alpha = 0.5;
	inputGen.SelectGenerator("uniform");
	outLinesC = inputGen.GetLinesFloat(width * sizeof(float), noLines, 99);
	outLinesAsm = inputGen.GetLinesFloat(width * sizeof(float), noLines, 99);
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	inMask = inputGen.GetLines(width, noLines, 0, 2);
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

TEST_F(AccumulateWeightedTest, TestRandomInputRandomMaskAlpha027891DestRandom)
{
	width = 160;
	alpha = 0.27891;
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width, noLines, 0, 255);
	inMask = inputGen.GetLines(width, noLines, 0, 2);
	outLinesC = inputGen.GetLinesFloat(width * sizeof(float), noLines, 0, 255);
	outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), noLines);
	for(int i = 0; i <width; i++)
	{
		outLinesAsm[0][i] = outLinesC[0][i];
	}
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);
	
	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(outLinesC[0][i], outLinesAsm[0][i]);
	}	
}

TEST_F(AccumulateWeightedTest, TestUniformInput0RandomMaskAlpha0333DestRandom)
{
	width = 640;
	alpha = 0.333;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width, noLines, 0);
	inputGen.SelectGenerator("random");
	inMask = inputGen.GetLines(width, noLines, 0, 2);
	outLinesC = inputGen.GetLinesFloat(width * sizeof(float), noLines, 0, 255);
	outLinesAsm = (float**)inputGen.GetEmptyLines(width * sizeof(float), noLines);
	for(int i = 0; i <width; i++)
	{
		outLinesAsm[0][i] = outLinesC[0][i];
	}
	
	AccumulateWeighted(inLines, inMask, outLinesC, width, alpha);
	AccumulateWeighted_asm_test(inLines, inMask, outLinesAsm, width, alpha);
	RecordProperty("CyclePerPixel", AccumulateWeightedCycleCount / width);	

	for(int i = 0; i < width; i++)
	{
		EXPECT_FLOAT_EQ(outLinesC[0][i], outLinesAsm[0][i]);
	}	
}
