//fast9Kernel test
//
// Test description:
// The assembly implementation of FAST9 doesn't exactly match the C implementation
// in the sense that it might detect fewer points or extra points compared to the C function.
// Therefore the tests check that a big percentage of the reference points are found by the optimized
// function. The current implementation of the checking function doesn't assume any ordering of the 
// points found.
//
//Asm function prototype:
//    void fast9_asm(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width);
//
//Asm test function prototype:
//    void fast9(unsigned char** in_lines, unsigned char* score,  unsigned int *base,
//               unsigned int posy, unsigned int thresh, unsigned int width);
//
//C function prototype:
//    void fast9(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width);
//
//Parameter description:
// in_lines   - array of pointers to input lines
// score      - pointer to corner score buffer
// base       - pointer to corner candidates buffer ; first element is the number of candidates, the rest are the position of coordinates
// posy       - y position
// thresh     - threshold
// width      - number of pixels to process


#include "gtest/gtest.h"
#include "feature.h"
#include "fast9_asm_test.h"
#include "fast9_unittest.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

using namespace mvcv;
using ::testing::TestWithParam;
using ::testing::Values;

#define PADDING 8
#define DELTA_PERCENT 95.0f

u8 inMatrix[7][16] =
{
	{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 9, 0, 0, 7, 0, 0, 9, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 9, 0, 8, 8, 8, 0, 9, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 9, 8, 8, 8, 9, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0}
};


class fast9Test : public ::testing::TestWithParam<unsigned int> {
 protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}
	ArrayChecker outputChecker;
	u8 **inBuff;
	u8** inLines;
	u8* scoreC;
	u8* scoreAsm;
	unsigned int *baseC;
	unsigned int *baseAsm;

	u32 width;
	u32 posy;
	u32 thresh;

    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;
    
    // Useful for debugging
    void showResults()
    {
        unsigned int maxCorners = baseAsm[0] > baseC[0] ? baseAsm[0] : baseC[0];
        
        if (scoreC != null && scoreAsm != null)
        {
            printf("score: \n");
            for (unsigned int i = 0; i < maxCorners; i++)
            {
                printf("%2d: %3d %3d\n", i, scoreC[i], scoreAsm[i]);
            }
            printf("\n");
        }

        if (baseC != null && baseAsm != null)
        {
            // We go to +1 because the first location encodes the number of corners found
            printf("base: \n");
            for (unsigned int i = 0; i < maxCorners + 1; i++)
            {
                printf("%2d: 0x%8x 0x%8x\n", i, baseC[i], baseAsm[i]);
            }
            printf("\n");
        }
    }
  // virtual void TearDown() {}
};

TEST_F(fast9Test , Test1)
{
	width = 32;
	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLines(width + PADDING, 7, 0);

	for(int i = 0; i < 7; i++)
	{
		for(int j = 0; j < 16; j++)
		{
			inLines[i][j] = inMatrix[i][j];
		}
	}

	scoreC = inputGen.GetEmptyLine(width);
	scoreAsm = inputGen.GetEmptyLine(width);
	baseC = (unsigned int*)inputGen.GetEmptyLine(width * 4);
	baseAsm = (unsigned int*)inputGen.GetEmptyLine(width * 4);

	posy = 3;
	thresh = 1;

	fast9_asm_test(inLines, scoreAsm, baseAsm, posy, thresh, width);
	fast9(inLines, scoreC, baseC, posy, thresh, width);
    
    //showResults();
    
    // The assembly implementation is not 100% matching the C one so we can admit a different number of corners
    // as long as those which are found are matching
	EXPECT_NEAR_PERCENT(baseC[0], baseAsm[0], 100.0f);
    CompareArrays((u16*)&baseC[1], (u16*)&baseAsm[1], baseC[0] * 2, baseAsm[0] * 2);
    CompareArrays(scoreC, scoreAsm, baseC[0], baseAsm[0]);
}

// //---------------- parameterized tests -------------------------

INSTANTIATE_TEST_CASE_P(RandomInputs, fast9Test,
		Values(160, 320, 640, 816, 1280, 1920);
);

TEST_P(fast9Test , TestRandomData)
{
	width = GetParam();
	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLines(width + PADDING, 7, 0, 255);

	scoreC = inputGen.GetEmptyLine(width);
	scoreAsm = inputGen.GetEmptyLine(width);
	baseC = (unsigned int*)inputGen.GetEmptyLine(width * 4);
	baseAsm = (unsigned int*)inputGen.GetEmptyLine(width * 4);

	posy = 3;
	thresh = 4;

	fast9_asm_test(inLines, scoreAsm, baseAsm, posy, thresh, width);
	fast9(inLines, scoreC, baseC, posy, thresh, width);

	//showResults();
    
	EXPECT_NEAR_PERCENT(baseC[0], baseAsm[0], DELTA_PERCENT);
    CompareArrays((u16*)&baseC[1], (u16*)&baseAsm[1], baseC[0] * 2, baseAsm[0] * 2, DELTA_PERCENT);
    CompareArrays(scoreC, scoreAsm, baseC[0], baseAsm[0], DELTA_PERCENT);
}
