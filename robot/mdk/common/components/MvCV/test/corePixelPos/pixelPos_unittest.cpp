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
#include "core.h"
#include "pixelPos_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;

class PixelPosTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
		pixelPositionC = 0;
        pixelPositionAsm = 0;
        statusC = 0;
        statusAsm = 0;
	}	
	
	unsigned char **inLines;
	float **outLinesC;
	float **outLinesAsm;
    unsigned char *mask;
    unsigned int width;
	unsigned long pixelPositionAsm;
	unsigned long pixelPositionC;
    unsigned char statusC;
    unsigned char statusAsm;
	

	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;

	virtual void TearDown() {}
};

// 1. Show position of '3'
TEST_F(PixelPosTest, TestUniformInputLine)
{
      width = 320;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      
      inLines[0][4] = 3;      
      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 3, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 3, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);
      
      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(4, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(4, pixelPositionC);
}

// 2. Check if it fails with max width
TEST_F(PixelPosTest, TestMaxWidth)
{
      width = 1920;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      
      inLines[0][50] = 3;
      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 3, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 3, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);
      
      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(50, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(50, pixelPositionC);
}

// 3. Check if it fails with min width
TEST_F(PixelPosTest, TestMinWidth)
{
      width = 8;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 5);
      
      inLines[0][5] = 3;
      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 3, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 3, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);
      
      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(5, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(5, pixelPositionC);
}

// 4. Check if it fails with pixelPosition on first pixel
TEST_F(PixelPosTest, TestFirstPixel)
{
      width = 32;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);

      inLines[0][0] = 3;
      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 3, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 3, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(0, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(0, pixelPositionC);
}

// 5. Check if it fails with pixelPosition on last pixel
TEST_F(PixelPosTest, TestLastPixel)
{
      width = 32;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);

      inLines[0][31]  = 27;
      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 27, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 27, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(31, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(31, pixelPositionC);
}

// 6. A mask pixel is set to 0, and the rest pixels are set on 1
TEST_F(PixelPosTest, TestSearchedPixelMasked)
{
      width = 24;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);

      mask = inputGen.GetLine(width, 1);
      inLines[0][22] = 125;
      mask[22]=0;

      pixelPos_asm_test(inLines, mask, width, 125, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 125, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0, statusAsm);
      EXPECT_EQ(0, pixelPositionAsm);

      EXPECT_EQ(0, statusC);
      EXPECT_EQ(0, pixelPositionC);
}

// 7. A mask pixel is set to 1, and the rest pixels are set on 0
TEST_F(PixelPosTest, TestOnlySearchedPixelNotMasked)
{
      width = 320;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);

      mask = inputGen.GetLine(width, 0);
      inLines[0][5] = 255;
      mask[5]=1;

      pixelPos_asm_test(inLines, mask, width, 255, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 255, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(5, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(5, pixelPositionC);
}

// 8. Same pixel on multiple positions
TEST_F(PixelPosTest, TestSamePixelOnMultiplePositions)
{
      width = 320;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      mask = inputGen.GetLine(width, 1);
      mask[4] = 0;
      
      inLines[0][4]   = 25;
      inLines[0][55]  = 25;
      inLines[0][120] = 25;
      inLines[0][319] = 25;

      pixelPos_asm_test(inLines, mask, width, 25, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 25, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);
      
      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(55, pixelPositionC);
      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(55, pixelPositionAsm);
}

// 9. 2 same pixels
TEST_F(PixelPosTest, TestTwoPixelsSameValue)
{
      width = 40;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);

      inLines[0][32] = 3;
      inLines[0][39] = 3;

      mask = inputGen.GetLine(width, 1);

      pixelPos_asm_test(inLines, mask, width, 3, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 3, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(32, pixelPositionAsm);
      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(32, pixelPositionC);
}

// 10. Generate random mask pixels, search after '4' and compare pixelPosition from Asm to the one from C
TEST_F(PixelPosTest, TestUniformInputRandomMask)
{
      width = 32;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 4);

      inputGen.SelectGenerator("random");
      mask = inputGen.GetLine(width, 0, 2);

      pixelPos_asm_test(inLines, mask, width, 4, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 4, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}

// 11. Random lines, random mask
TEST_F(PixelPosTest, TestRandomInputRandomMaskWidth8)
{
      width = 8;
      inputGen.SelectGenerator("random");
      inLines = inputGen.GetLines(width, 1, 0, 255);
      mask = inputGen.GetLine(width, 0, 2);
      
      unsigned char searchedPixel = inLines[0][randomGen.GenerateUChar(0, width)];
      pixelPos_asm_test(inLines, mask, width, searchedPixel, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, searchedPixel, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}

// 12. Random lines, random mask
TEST_F(PixelPosTest, TestRandomInputRandomMaskWidth640)
{
      width = 640;
      inputGen.SelectGenerator("random");
      inLines = inputGen.GetLines(width, 1, 0, 255);
      mask = inputGen.GetLine(width, 0, 2);

      unsigned char searchedPixel = randomGen.GenerateUChar(0, 255);
      pixelPos_asm_test(inLines, mask, width, searchedPixel, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, searchedPixel, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}

// 13. Random lines, random mask
TEST_F(PixelPosTest, TestRandomInputRandomMaskWidth800)
{
      width = 800;
      inputGen.SelectGenerator("random");
      inLines = inputGen.GetLines(width, 1, 0, 255);
      mask = inputGen.GetLine(width, 0, 2);

      unsigned char searchedPixel = randomGen.GenerateUChar(0, 255);
      pixelPos_asm_test(inLines, mask, width, searchedPixel, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, searchedPixel, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}

// 14. Random lines, random mask
TEST_F(PixelPosTest, TestRandomInputRandomMaskWidth1280)
{
      width = 1280;
      inputGen.SelectGenerator("random");
      inLines = inputGen.GetLines(width, 1, 0, 255);
      mask = inputGen.GetLine(width, 0, 2);

      unsigned char searchedPixel = randomGen.GenerateUChar(0, 255);
      pixelPos_asm_test(inLines, mask, width, searchedPixel, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, searchedPixel, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}


// 15. Random lines, random mask
TEST_F(PixelPosTest, TestRandomInputRandomMaskWidth1920)
{
      width = 1920;
      inputGen.SelectGenerator("random");
      inLines = inputGen.GetLines(width, 1, 0, 255);
      mask = inputGen.GetLine(width, 0, 2);

      unsigned char searchedPixel = randomGen.GenerateUChar(0, 255);
      pixelPos_asm_test(inLines, mask, width, searchedPixel, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, searchedPixel, &pixelPositionC, &statusC);
      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(statusC, statusAsm);
      EXPECT_EQ(pixelPositionC, pixelPositionAsm);
}

// 16. Searching for 7 when are more pixels with value=7
TEST_F(PixelPosTest, TestMorePixelsSameValueAllMasked)
{
      width = 40;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      mask = inputGen.GetLine(width, 1);

      inLines[0][7]  = 7; mask[7]  = 0;
      inLines[0][9]  = 7; mask[9]  = 0;
      inLines[0][16] = 7; mask[16] = 0;
      inLines[0][21] = 7; mask[21] = 0;

      pixelPos_asm_test(inLines, mask, width, 7, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 7, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0, statusAsm);
      EXPECT_EQ(0, pixelPositionAsm);

      EXPECT_EQ(0, statusC);
      EXPECT_EQ(0, pixelPositionC);
}

// 17. Searching for a pixel with value 0
TEST_F(PixelPosTest, TestSearchForPixel0)
{
      width = 20;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      mask = inputGen.GetLine(width, 1);
      
      mask[0] = 0;
//      mask[17] = 0;
      inLines[0][5] = 0;

      pixelPos_asm_test(inLines, mask, width, 0, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 0, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(5, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(5, pixelPositionC);
}

// 18. Searching for a pixel with value 0 in compensation part
TEST_F(PixelPosTest, TestSearchForPixel0Compensation)
{
      width = 20;
      inputGen.SelectGenerator("uniform");
      inLines = inputGen.GetLines(width, 1, 15);
      mask = inputGen.GetLine(width, 1);
      
      mask[0] = 0;
      mask[17] = 0;
      inLines[0][18] = 0;

      pixelPos_asm_test(inLines, mask, width, 0, &pixelPositionAsm, &statusAsm);
      pixelPos(inLines, mask, width, 0, &pixelPositionC, &statusC);

      RecordProperty("CycleCount", PixelPosCycleCount);

      EXPECT_EQ(0x11, statusAsm);
      EXPECT_EQ(18, pixelPositionAsm);

      EXPECT_EQ(0x11, statusC);
      EXPECT_EQ(18, pixelPositionC);
}
