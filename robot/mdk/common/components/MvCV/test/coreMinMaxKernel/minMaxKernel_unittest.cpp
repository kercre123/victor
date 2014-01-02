//minMaxKernel test
//Asm function prototype:
//    void minMaxKernel_asm(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
//
//Asm test function prototype:
//    void minMaxKernel_asm_test(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
//    - restrictions: works only for line size multiple of 16
//
//C function prototype:
//    void minMaxKernel(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
//
//Parameter description:
//    in - input lines
//    width - input lines width
//    height - number of input lines
//    minVal - returns minimum pixel value
//    minVal - returns maximum pixel value
//    maskAddr - pixel mask: only pixels having mask value of 1 will be processed
//
//Test cases to be implemented
//    1. uniform values for the entire line
//       - expected result: both min and max have the same value with that in the input line
//    2. nominal test: minimum and maximum located in various position in the input line
//       - expected result: min and max are calculated correctly by both C and asm functions
//    3. boundary tests: check if minimum and maximum are correctly identified when located on the first or last position in the input line
//       - expected result: min and max are calculated correctly by both C and asm functions
//    4. different mask patterns:
//       - expected results: min and max are calculated correctly, considering mask configuration
//    5. different input line sizes:
//       - at least one test for each common line size: 320, 680, 800, 1280, 1920
//    6. random input values and random mask values
//        - expected results: output results are the same for both C and asm functions

#include "gtest/gtest.h"
#include "core.h"
#include "minMaxKernel_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "FunctionInfo.h"
#include <ctime>
#include <memory>

using namespace mvcv;

class MinMaxKernelTest : public ::testing::Test {
 protected:

    virtual void SetUp()
    {
        asmMinVal = 255;
        asmMaxVal = 0;
        cMinVal = 255;
        cMaxVal = 0;
        randGen.reset(new RandomGenerator);
        uniGen.reset(new UniformGenerator);
    	inputGen.AddGenerator(std::string("random"), randGen.get());
    	inputGen.AddGenerator(std::string("uniform"), uniGen.get());
    }


    u8 asmMinVal;
    u8 asmMaxVal;
    u8 cMinVal;
    u8 cMaxVal;
    u32 width;
    InputGenerator inputGen;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;
    unsigned char** inputLines;
    unsigned char* mask;

  //virtual void TearDown() {}
};

TEST_F(MinMaxKernelTest, TestUniformInputLine)
{

  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(320, 1, 15);
  mask = inputGen.GetLine(320, 1);
  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  
  EXPECT_EQ(15, asmMinVal);
  EXPECT_EQ(15, asmMaxVal);
}



TEST_F(MinMaxKernelTest, TestAsmAllValues255) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(320, 1, 255);
  mask = inputGen.GetLine(320, 1);

  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(255, asmMinVal);
  EXPECT_EQ(255, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestAsmAllValuesZero) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(320, 1, 0);
  mask = inputGen.GetLine(320, 1);

  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(0, asmMinVal);
  EXPECT_EQ(0, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestAsmAllValuesMasked) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(320, 1, 20);
  mask = inputGen.GetLine(320, 1);
  inputLines[0][301] = 127;
  inputLines[0][25] = 5;

  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(5, asmMinVal);
  EXPECT_EQ(127, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestAsmLowestLineSize) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(16, 1, 20);
  mask = inputGen.GetLine(16, 1);
  inputLines[0][5] = 100;
  inputLines[0][15] = 2;

  minMaxKernel_asm_test(inputLines, 16, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(2, asmMinVal);
  EXPECT_EQ(100, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestLefmostMinRightmostMax) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(320, 1, 20);
  mask = inputGen.GetLine(320, 1);
  inputLines[0][0] = 1;
  inputLines[0][319] = 254;

  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(1, asmMinVal);
  EXPECT_EQ(254, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestLefmostMaxRightmostMin) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(640, 1, 20);
  mask = inputGen.GetLine(640, 1);
  inputLines[0][0] = 50;
  inputLines[0][639] = 0;

  minMaxKernel_asm_test(inputLines, 640, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(0, asmMinVal);
  EXPECT_EQ(50, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestMinAndMaxValuesUnmasked) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(640, 1, 20);
  mask = inputGen.GetLine(640, 1);

  inputLines[0][120] = 50;
  inputLines[0][121] = 49;
  inputLines[0][240] = 0;
  inputLines[0][241] = 1;
  mask[120] = 0;
  mask[240] = 0;

  minMaxKernel_asm_test(inputLines, 640, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(1, asmMinVal);
  EXPECT_EQ(49, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestOnlyOneMaskedValue) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(640, 1, 20);
  mask = inputGen.GetLine(640, 0);

  mask[230] = 1;
  inputLines[0][230] = 7;
  
  minMaxKernel_asm_test(inputLines, 640, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(7, asmMinVal);
  EXPECT_EQ(7, asmMaxVal);
}



TEST_F(MinMaxKernelTest, TestAllValuesUnmasked) {
  inputGen.SelectGenerator("uniform");
  inputLines = inputGen.GetLines(640, 1, 20);
  mask = inputGen.GetLine(640, 0);

  inputLines[0][230] = 7;
  inputLines[0][630] = 220;
  
  minMaxKernel_asm_test(inputLines, 640, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);

  EXPECT_EQ(255, asmMinVal);
  EXPECT_EQ(0, asmMaxVal);
}


TEST_F(MinMaxKernelTest, TestRandomDataSize320){
  inputGen.SelectGenerator("random");
  inputLines = inputGen.GetLines(320, 1, 0, 255);
  mask = inputGen.GetLine(320, 0, 2);

  minMaxKernel_asm_test(inputLines, 320, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  minMaxKernel(inputLines, 320, 1, &cMinVal, &cMaxVal, mask);

  EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
  EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
}


TEST_F(MinMaxKernelTest, TestRandomDataSize640){
  inputGen.SelectGenerator("random");
  inputLines = inputGen.GetLines(640, 1, 0, 255);
  mask = inputGen.GetLine(640, 0, 2);

  minMaxKernel_asm_test(inputLines, 640, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  minMaxKernel(inputLines, 640, 1, &cMinVal, &cMaxVal, mask);

  EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
  EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
}

TEST_F(MinMaxKernelTest, TestRandomDataSize800){
  inputGen.SelectGenerator("random");
  inputLines = inputGen.GetLines(800, 1, 0, 255);
  mask = inputGen.GetLine(800, 0, 2);

  minMaxKernel_asm_test(inputLines, 800, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  minMaxKernel(inputLines, 800, 1, &cMinVal, &cMaxVal, mask);

  EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
  EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
}


TEST_F(MinMaxKernelTest, TestRandomDataSize1280){
  inputGen.SelectGenerator("random");
  inputLines = inputGen.GetLines(1280, 1, 0, 255);
  mask = inputGen.GetLine(1280, 0, 2);

  minMaxKernel_asm_test(inputLines, 1280, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  minMaxKernel(inputLines, 1280, 1, &cMinVal, &cMaxVal, mask);

  EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
  EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
}

TEST_F(MinMaxKernelTest, TestRandomDataSize1920){
  inputGen.SelectGenerator("random");
  inputLines = inputGen.GetLines(1920, 1, 0, 255);
  mask = inputGen.GetLine(1920, 0, 2);

  minMaxKernel_asm_test(inputLines, 1920, 1, &asmMinVal, &asmMaxVal, mask);
  RecordProperty("CycleCount", minMaxCycleCount);
  minMaxKernel(inputLines, 1920, 1, &cMinVal, &cMaxVal, mask);

  EXPECT_TRUE(cMinVal == asmMinVal) << "cMinVal: " << (unsigned int)cMinVal << " asmMinVal: " << (unsigned int)asmMinVal;
  EXPECT_TRUE(cMaxVal == asmMaxVal) << "cMaxVal: " << (unsigned int)cMaxVal << " asmMaxVal: " << (unsigned int)asmMaxVal;;
}
