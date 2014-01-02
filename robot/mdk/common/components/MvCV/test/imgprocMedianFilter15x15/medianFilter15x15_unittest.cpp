//medianFilter15x15 kernel test

//Asm function prototype:
//      void medianFilter15x15(u32 widthLine, u8 **outLine, u8 ** inLine)

//Asm test function prototype:
//      void medianFilter15x15_asm_test(unsigned int widthLine, unsigned char** outLine, unsigned char** inLine)

//C function prototype:
//      void medianFilter15x15(u32 widthLine, u8 **outLine, u8 ** inLine)

//widthLine     - width of input line
//outLine		- array of pointers for output lines
//inLine 		- array of pointers to input lines

#include "gtest/gtest.h"
#include "imgproc.h"
#include "medianFilter15x15_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

#define PADDING 14
#define DELTA 1

using namespace mvcv;

unsigned char inLinesTest[15][15]  = {{1, 9, 3, 4, 5, 6, 7, 8, 9, 4, 5, 2, 4, 7, 9},
                                      {2, 8, 9, 9, 7, 3, 3, 9, 9, 5, 2, 1, 0, 6, 5},
                                      {3, 7, 2, 1, 0, 3, 4, 4, 8, 5, 1, 1, 9, 4, 7},
                                      {4, 6, 5, 4, 3, 2, 0, 0, 8, 2, 5, 3, 5, 7, 9},
                                      {5, 5, 4, 4, 7, 7, 1, 1, 3, 1, 6, 1, 2, 9, 8},
                                      {6, 4, 8, 8, 7, 7, 2, 2, 9, 4, 9, 6, 4, 7, 8},
                                      {7, 3, 6, 6, 7, 7, 9, 9, 8, 2, 3, 3, 2, 7, 5},
                                      {8, 2, 6, 7, 8, 3, 5, 9, 7, 5, 1, 3, 6, 9, 0},
                                      {9, 1, 2, 3, 7, 1, 0, 9, 8, 4, 3, 0, 3, 6, 2},
                                      {8, 2, 6, 7, 8, 3, 5, 9, 7, 5, 1, 1, 4, 9, 5},
                                      {9, 1, 2, 3, 7, 1, 0, 9, 8, 4, 3, 2, 1, 4, 8},
									  {1, 3, 5, 2, 4, 8, 6, 5, 8, 0, 9, 2, 4, 7, 3},
									  {3, 2, 5, 7, 1, 9, 8, 0, 4, 6, 2, 2, 3, 1, 7},
									  {1, 4, 6, 7, 3, 2, 5, 8, 9, 0, 1, 5, 6, 7, 2},
									  {1, 5, 6, 9, 0, 2, 4, 5, 3, 2, 7, 8, 5, 4, 6}};


class medianFilter15x15Test : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	

    unsigned char** inLines;
    unsigned char** outLinesC;
	unsigned char** outLinesAsm;
    unsigned char** aux;
    unsigned char** inLinesC;
	unsigned int width;
    unsigned int height;
	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};


// 1. all pixels value = 0 - min

TEST_F(medianFilter15x15Test, TestUniformInput0Size15x15)
{
    width = 15;
    height = 15;

    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width + PADDING, height, 0);

    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

// 2. all pixels value = 255 - max
TEST_F(medianFilter15x15Test, TestUniformInput255Size15x15)
{
    width = 15;
    height = 15;

	inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width + PADDING, height, 255);
    outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

	for(int i = 0; i < height; i++)
	{
        for(int j = 0; j < PADDING / 2; j++)
        {
            inLines[i][j] = 0;
            inLines[i][width + PADDING - j - 1] = 0;
        }
	}
		
    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

//3. all pixels value = 198 - between min and max
TEST_F(medianFilter15x15Test, TestUniformInput198Size15x15)
{
    width = 15;
    height = 15;

    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width + PADDING, height, 198);
	outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);
    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < PADDING / 2; j++)
        {
            inLines[i][j] = 0;
            inLines[i][width + PADDING - j - 1] = 0;
        }
    }
	
    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

// 4. all pixels value = pseudo random numbers
TEST_F(medianFilter15x15Test, TestPseudoRandomInputSize15x15)
{
    width = 15;
    height = 15;
    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width + PADDING, height, 0);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
            inLines[i][j + (PADDING / 2)] = inLinesTest[i][j];


    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width, DELTA);
}

// 5. all pixels value = random, width=8    11?
TEST_F(medianFilter15x15Test, TestRandomInputSize8x15)
{
    width = 8;
    height = 15;

    inputGen.SelectGenerator("uniform");
    inLines = inputGen.GetLines(width + PADDING , height, 1);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.ExpectAllEQ<unsigned char>(outLinesAsm[0], width, 1);
}

// 6. all pixels value = random, width=15
TEST_F(medianFilter15x15Test, TestRandomInputSize15x15)
{
    width = 15;
    height = 15;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


// 7. all pixels value = random, width=640
TEST_F(medianFilter15x15Test, TestRandomInputSize640x15)
{
    width = 640;
    height = 15;
    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}


// 8. all pixels value = random, width=800
TEST_F(medianFilter15x15Test, TestRandomInputSize800x15)
{
    width = 800;
    height = 15;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width + PADDING, height, 0, 255);

    outLinesC = inputGen.GetEmptyLines(width, 1);
	outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

	outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

// 9. all pixels value = random, width=1200
TEST_F(medianFilter15x15Test, TestRandomInputSize1200x15)
{
    width = 1200;
    height = 15;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}

// 10. all pixels value = random, width=1920
TEST_F(medianFilter15x15Test, TestRandomInputSize1920x15)
{
    width = 1920;
    height = 15;

    inputGen.SelectGenerator("random");
    inLines = inputGen.GetLines(width + PADDING, height, 0, 255);
    outLinesC = inputGen.GetEmptyLines(width, 1);
    outLinesAsm = inputGen.GetEmptyLines(width, 1);

    inLinesC = inputGen.GetOffsettedLines(inLines, width, PADDING / 2);

    medianFilter15x15(width, outLinesC, inLinesC);
    medianFilter15x15_asm_test(width, outLinesAsm, inLines);

    RecordProperty("CyclePerPixel", medianFilter15x15CycleCount / width);

    outputCheck.CompareArrays(outLinesC[0], outLinesAsm[0], width);
}
