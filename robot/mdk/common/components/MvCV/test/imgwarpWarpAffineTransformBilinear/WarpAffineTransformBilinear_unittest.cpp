//WarpAffineTransformBilinear kernel test

//Asm function prototype:
//	    void WarpAffineTransformBilinear_asm(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width,
//                           u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, half mat[2][3]);
//
//Asm test function prototype:
//    	void WarpAffineTransformBilinear_asm_test(unsigned char** in, unsigned char** out, unsigned int in_width,
//		unsigned int in_height, unsigned int out_width, unsigned int out_height,
//		unsigned int in_stride, unsigned in out_stride, unsigned char* fill_color, float imat[2][3]);
//
//C function prototype:
//  	    void WarpAffineTransformBilinear(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width,
//                               u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, float imat[2][3]);
//
// WarpAffineTransform kernel
// in         - address of pointer to input frame
// out        - address of pointer to input frame
// in_width   - width of input frame
// in_height  - height of input frame
// out_width  - width of output frame
// out_height - height of output frame
// in_stride  - stride of input frame
// out_stride - stride of output frame
// fill_color - pointer to filled color value (if NULL no fill color)
// mat        - inverted matrix from warp transformation

#include "gtest/gtest.h"
#include "imgwarp.h"
#include "WarpAffineTransformBilinear_asm_test.h"
#include "RandomGenerator.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "TestEventListener.h"
#include "ArrayChecker.h"
#include <ctime>

using namespace mvcv;

class WarpAffineBilinearTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}	
	
	unsigned char *inLines;
	unsigned char *outLinesC;
	unsigned char *outLinesAsm;
	unsigned int in_width;
	unsigned int in_height;
	unsigned int in_stride;
	unsigned int out_width;
	unsigned int out_height;
	unsigned int out_stride;
	unsigned char fill_color;
	float iMatC[2][3];
	half iMatAsm[2][3];

	InputGenerator inputGen;
	RandomGenerator randomGen;
	ArrayChecker outputCheck;
	
	std::auto_ptr<RandomGenerator>  randGen;
	std::auto_ptr<UniformGenerator>  uniGen;
	ArrayChecker outputChecker;

	virtual void TearDown() {}
};


TEST_F(WarpAffineBilinearTest, TestUniformInputLines)
{
	in_width = 32;
	in_height = 8;
	out_width = 32;
	out_height = 8;
	in_stride = 32;
	out_stride = 32;
	fill_color = 7;
	
	iMatC[0][0] = 1.0f; iMatC[0][1] = 0.0f; iMatC[0][2] = 0.0f;
	iMatC[1][0] = 0.0f; iMatC[1][1] = 1.0f; iMatC[1][2] = 0.0f;

	iMatAsm[0][0] = 1.0; iMatAsm[0][1] = 0.0; iMatAsm[0][2] = 0.0;
	iMatAsm[1][0] = 0.0; iMatAsm[1][1] = 1.0; iMatAsm[1][2] = 0.0;


	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(in_width * in_height, 10);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);
    
    
	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);

	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}

TEST_F(WarpAffineBilinearTest, TestUniformInputLinesRot180)
{
	in_width = 32;
	in_height = 8;
	out_width = 32;
	out_height = 8;
	in_stride = 32;
	out_stride = 32;
	fill_color = 5;
	
	iMatC[0][0] = -1.0f; iMatC[0][1] = 0.0f; iMatC[0][2] = 320.0f;
	iMatC[1][0] = 0.0f; iMatC[1][1] = -1.0f; iMatC[1][2] = (float)in_height;

	iMatAsm[0][0] = -1.0; iMatAsm[0][1] = 0.0; iMatAsm[0][2] = 320;
	iMatAsm[1][0] = 0.0; iMatAsm[1][1] = -1.0; iMatAsm[1][2] = (half)in_height;


	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(in_width * in_height, 20);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);

    
	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);
    
	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}

TEST_F(WarpAffineBilinearTest, TestUniformInputLinesRot90)
{
	in_width = 32;
	in_height = 8;
	out_width = 32;
	out_height = 8;
	in_stride = 32;
	out_stride = 32;
	fill_color = 7;
	
	iMatC[0][0] = 0.0f; iMatC[0][1] = -1.0f; iMatC[0][2] = 320.0f;
	iMatC[1][0] = 1.0f; iMatC[1][1] =  0.0f; iMatC[1][2] = 0.0f;

	iMatAsm[0][0] = 0.0; iMatAsm[0][1] = -1.0; iMatAsm[0][2] = 320.0;
	iMatAsm[1][0] = 1.0; iMatAsm[1][1] =  0.0; iMatAsm[1][2] = 0.0;


	inputGen.SelectGenerator("uniform");
	inLines = inputGen.GetLine(in_width * in_height, 30);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);


	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);
    
	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}


TEST_F(WarpAffineBilinearTest, TestRandomInput)
{
	in_width = 32;
	in_height = 8;
	out_width = 32;
	out_height = 8;
	in_stride = 32;
	out_stride = 32;
	fill_color = 5;
	
	iMatC[0][0] = 1.0f; iMatC[0][1] = 0.0f; iMatC[0][2] = 0.0f;
	iMatC[1][0] = 0.0f; iMatC[1][1] = 1.0f; iMatC[1][2] = 0.0f;

	iMatAsm[0][0] = 1.0; iMatAsm[0][1] = 0.0; iMatAsm[0][2] = 0.0;
	iMatAsm[1][0] = 0.0; iMatAsm[1][1] = 1.0; iMatAsm[1][2] = 0.0;


	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLine(in_width * in_height, 0, 255);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);
    

	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);

	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}

TEST_F(WarpAffineBilinearTest, TestRandomInputRot90)
{
	in_width = 64;
	in_height = 16;
	out_width = 64;
	out_height = 16;
	in_stride = 64;
	out_stride = 64;
	fill_color = 5;
	
	iMatC[0][0] = 0.0f; iMatC[0][1] = -1.0f; iMatC[0][2] = 320.0f;
	iMatC[1][0] = 1.0f; iMatC[1][1] =  0.0f; iMatC[1][2] = 0.0f;

	iMatAsm[0][0] = 0.0; iMatAsm[0][1] = -1.0; iMatAsm[0][2] = 320.0;
	iMatAsm[1][0] = 1.0; iMatAsm[1][1] =  0.0; iMatAsm[1][2] = 0.0;


	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLine(in_width * in_height, 0, 255);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);
    

	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);

	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}

TEST_F(WarpAffineBilinearTest, TestRandomInputRot180)
{
	in_width = 24;
	in_height = 32;
	out_width = 24;
	out_height = 32;
	in_stride = 24;
	out_stride = 24;
	fill_color = 5;
	
	iMatC[0][0] = -1.0f; iMatC[0][1] = 0.0f; iMatC[0][2] = 320.0f;
	iMatC[1][0] = 0.0f; iMatC[1][1] = -1.0f; iMatC[1][2] = (float)in_height;

	iMatAsm[0][0] = -1.0; iMatAsm[0][1] = 0.0; iMatAsm[0][2] = 320;
	iMatAsm[1][0] = 0.0; iMatAsm[1][1] = -1.0; iMatAsm[1][2] = (half)in_height;


	inputGen.SelectGenerator("random");
	inLines = inputGen.GetLine(in_width * in_height, 0, 255);
	outLinesC = inputGen.GetEmptyLine(out_width * out_height);
	outLinesAsm = inputGen.GetEmptyLine(out_width * out_height);
    

	WarpAffineTransformBilinear_asm_test((unsigned char**)&inLines, (unsigned char**)&outLinesAsm, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatAsm);
	WarpAffineTransformBilinear((unsigned char**)&inLines, (unsigned char**)&outLinesC, in_width, in_height,
			out_width, out_height, in_stride, out_stride, &fill_color, iMatC);

	RecordProperty("CycleCount", warpAffineBilinearCycleCount);

	outputChecker.CompareArrays(outLinesC, outLinesAsm, out_width * out_height);
}

