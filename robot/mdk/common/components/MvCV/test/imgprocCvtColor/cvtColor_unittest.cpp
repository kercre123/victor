//cvtColorKernel test
//Asm function prototype:
//    void cvtColorKernelYUVToRGB_asm(u8* yIn, u8* uIn, u8* vIn, u8* out, u32 width);
//    void cvtColorKernelRGBToYUV_asm(u8* in, u8* yOut, u8* uOut, u8* vOut, u32 width);
//
//Asm test function prototype:
//    void YUVtoRGB_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width)
//    void RGBtoYUV_asm_test(unsigned char *rgb, unsigned char *y ,unsigned char *u ,unsigned char *v , unsigned int width)
//
//C function prototype:
//    void cvtColorKernelYUVToRGB(u8* yIn, u8* uIn, u8* vIn, u8* out, u32 width);
//    void cvtColorKernelRGBToYUV(u8* in, u8* yOut, u8* uOut, u8* vOut, u32 width);
//
//Parameter description:
// YUVToRGB
// yIn input Y channel
// uIn input U channel
// vIn input V channel
// out output RGB channel
// width - image width in pixels
// RGBToYUV
// in input RGB channel
// yOut output Y channel
// uOut output U channel
// vOut output V channel
// width - image width in pixels


#include "gtest/gtest.h"
#include "imgproc.h"
#include "cvtColor_asm_test.h"
#include "InputGenerator.h"
#include "UniformGenerator.h"
#include "RandomGenerator.h"
#include <ctime>
#include <memory>
#include <stdlib.h>
#include "ArrayChecker.h"

using namespace mvcv;

class cvtColorTest : public ::testing::Test {
 protected:

	virtual void SetUp()
	{
		randGen.reset(new RandomGenerator);
		uniGen.reset(new UniformGenerator);
		inputGen.AddGenerator(std::string("random"), randGen.get());
		inputGen.AddGenerator(std::string("uniform"), uniGen.get());
	}

	void YUVtoRGBcheck(u8 r, u8 g, u8 b, int size, u8 *RGB)
	{
		int a;
		for(int i=0;i<size*3;i+=3)
			{
				if(RGB[i]>r)
					a=RGB[i]-r;
				else
					a=r-RGB[i];
				EXPECT_LE(a,2);
				if(RGB[i+1]>g)
					a=RGB[i+1]-g;
				else
					a=g-RGB[i+1];
				EXPECT_LE(a,2);
				if(RGB[i+2]>b)
					a=RGB[i+2]-b;
				else
					a=b-RGB[i+2];
				EXPECT_LE(a,2);
			}



	}



	void RGBtoYUVcheck(u8 *Y, u8 *U, u8 *V, int size, u8 y, u8 u, u8 v)
	{
		int i,a;
		for(i=0;i<size;i++)
		{
			if(Y[i]>y)
				a=Y[i]-y;
			else
				a=y-Y[i];
			EXPECT_LE(a,2);
		}
		for(i=0;i<size/4;i++)
		{
			if(U[i]>u)
				a=U[i]-u;
			else
				a=u-U[i];
			EXPECT_LE(a,2);
		}
		for(i=0;i<size/4;i++)
		{
			if(V[i]>v)
				a=V[i]-v;
			else
				a=v-V[i];
			EXPECT_LE(a,2);
		}
	}

	void DeltaCheck(u8* A, u8* B, int size)
	{
		int i,m;
		for(i=0;i<size;i++)
		{
			if(A[i]>=B[i])
				m=A[i]-B[i];
			else
				m=B[i]-A[i];
			EXPECT_LE(m,2);
		}
	}

    u8* y;
    u8* u;
    u8* v;
    u8* yASM;
    u8* uASM;
    u8* vASM;
    u8* rgb;
    u8* rgbASM;
    u8** in;

    RandomGenerator dataGenerator;
    InputGenerator inputGen;
    ArrayChecker arrCheck;
    std::auto_ptr<RandomGenerator>  randGen;
    std::auto_ptr<UniformGenerator>  uniGen;

  // virtual void TearDown() {}
};


TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGB1)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(32, 38);
	u = inputGen.GetLine(8, 5);
	v = inputGen.GetLine(8, 9);
	rgbASM = inputGen.GetEmptyLine(96);
	rgb = inputGen.GetEmptyLine(96);
	int size=32;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 16);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 16);
	RecordProperty("CycleCount", cvtColorCycleCount);


	arrCheck.CompareArrays(rgbASM,rgb,96);

	YUVtoRGBcheck(0, 165, 0, size, rgbASM);

	YUVtoRGBcheck(0, 165, 0, size, rgb);

}

TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGB2)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(128);
	u = inputGen.GetEmptyLine(32);
	v = inputGen.GetEmptyLine(32);
	rgbASM = inputGen.GetEmptyLine(384);
	rgb = inputGen.GetEmptyLine(384);
	int size=128,i;

	for(i=0;i<128; i++)
	{
		y[i]=i*2;
	}

	for(i=0;i<32; i++)
	{
		u[i]=i*4;
		v[i]=i*5;
	}

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 64);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 64);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,384);

}


TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUV1)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(32);
	u = inputGen.GetEmptyLine(8);
	v = inputGen.GetEmptyLine(8);
	yASM = inputGen.GetEmptyLine(32);
	uASM = inputGen.GetEmptyLine(8);
	vASM = inputGen.GetEmptyLine(8);
	rgb = inputGen.GetLine(96,55);
	int size = 32;

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 16);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 16);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,32,55,128,128);

	RGBtoYUVcheck(y,u,v,32,55,128,128);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);

}


TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUV2)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(32);
	u = inputGen.GetEmptyLine(8);
	v = inputGen.GetEmptyLine(8);
	yASM = inputGen.GetEmptyLine(32);
	uASM = inputGen.GetEmptyLine(8);
	vASM = inputGen.GetEmptyLine(8);
	rgb = inputGen.GetLine(96,184);

	int i=0;

	int size=32;
	for(i=0;i<96; i++)
	{
		if(i % 3 == 1)
		{
			rgb[i] = 77;
		}
		if(i % 3 == 2)
		{
			rgb[i] = 120;
		}
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 16);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 16);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,32,114,131,177);

	RGBtoYUVcheck(y,u,v,32,114,131,177);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);
}

TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVnullU)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(32);
	u = inputGen.GetEmptyLine(8);
	v = inputGen.GetEmptyLine(8);
	yASM = inputGen.GetEmptyLine(32);
	uASM = inputGen.GetEmptyLine(8);
	vASM = inputGen.GetEmptyLine(8);
	rgb = inputGen.GetLine(96,184);

	int i=0;

	int size=32;
	for(i=0;i<96; i++)
	{
		if(i % 3 == 1)
		{
			rgb[i] = 77;
		}
		if(i % 3 == 2)
		{
			rgb[i] = 120;
		}
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 16);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 16);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,32,114,131,177);

	RGBtoYUVcheck(y,u,v,32,114,131,177);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);
}

TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVNullR)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(32);
	u = inputGen.GetEmptyLine(8);
	v = inputGen.GetEmptyLine(8);
	yASM = inputGen.GetEmptyLine(32);
	uASM = inputGen.GetEmptyLine(8);
	vASM = inputGen.GetEmptyLine(8);
	rgb = inputGen.GetLine(96,0);

	int i=0;

	int size=32;
	for(i=0;i<96; i++)
	{
		if(i % 3 == 1)
		{
			rgb[i] = 167;
		}
		if(i % 3 == 2)
		{
			rgb[i] = 20;
		}
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 16);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 16);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,32,100,82,56);

	RGBtoYUVcheck(y,u,v,32,100,82,56);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);
}

TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVNullG)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(80);
	u = inputGen.GetEmptyLine(20);
	v = inputGen.GetEmptyLine(20);
	yASM = inputGen.GetEmptyLine(80);
	uASM = inputGen.GetEmptyLine(20);
	vASM = inputGen.GetEmptyLine(20);
	rgb = inputGen.GetLine(240,67);

	int i=0;

	int size=32;
	for(i=0;i<240; i++)
	{
		if(i % 3 == 1)
		{
			rgb[i] = 0;
		}
		if(i % 3 == 2)
		{
			rgb[i] = 200;
		}
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 40);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 40);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,80,42,216,145);

	RGBtoYUVcheck(y,u,v,80,42,216,145);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);
}


TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVNullB)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(132);
	u = inputGen.GetEmptyLine(33);
	v = inputGen.GetEmptyLine(33);
	yASM = inputGen.GetEmptyLine(132);
	uASM = inputGen.GetEmptyLine(33);
	vASM = inputGen.GetEmptyLine(33);
	rgb = inputGen.GetLine(396,196);

	int i=0;

	int size=132;

	i=0;


	for(i=0;i<396; i++)
	{
		if(i % 3 == 1)
		{
			rgb[i] = 180;
		}
		if(i % 3 == 2)
		{
			rgb[i] = 0;
		}
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 66);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 66);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,132,164,35,150);
	RGBtoYUVcheck(y,u,v,132,165,35,150);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);

}


TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVNullInput)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(64);
	u = inputGen.GetEmptyLine(16);
	v = inputGen.GetEmptyLine(16);
	yASM = inputGen.GetEmptyLine(64);
	uASM = inputGen.GetEmptyLine(16);
	vASM = inputGen.GetEmptyLine(16);
	rgb = inputGen.GetLine(192,0);
	int size=64;

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 32);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 32);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,64,0,128,128);

	RGBtoYUVcheck(y,u,v,64,0,128,128);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);

}

TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBNullY)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(256);
	u = inputGen.GetEmptyLine(64);
	v = inputGen.GetEmptyLine(64);
	rgbASM = inputGen.GetEmptyLine(768);
	rgb = inputGen.GetEmptyLine(768);
	int size=256,i;

	for(i=0;i<256; i++)
	{
		y[i]=0;
	}

	for(i=0;i<64; i++)
	{
		u[i]=i*2;
		v[i]=i*4;
	}

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 128);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 128);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,768);

}

TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBNullU)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(128);
	u = inputGen.GetEmptyLine(32);
	v = inputGen.GetEmptyLine(32);
	rgbASM = inputGen.GetEmptyLine(384);
	rgb = inputGen.GetEmptyLine(384);
	int size=128,i;

	for(i=0;i<128; i++)
	{
		y[i]=i*2;
	}

	for(i=0;i<32; i++)
	{
		u[i]=i*5;
		v[i]=0;
	}

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 64);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 64);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,384);

}


TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBNullV)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(400);
	u = inputGen.GetEmptyLine(100);
	v = inputGen.GetEmptyLine(100);
	rgbASM = inputGen.GetEmptyLine(1200);
	rgb = inputGen.GetEmptyLine(1200);
	int size=400,i;

	for(i=0;i<400; i++)
	{
		y[i]=i/2;
	}

	for(i=0;i<100; i++)
	{
		u[i]=i*2;
		v[i]=0;
	}

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 200);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 200);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,1200);

}


TEST_F(cvtColorTest , TestUniformInputLinYUVtoRGBNullInput)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(320, 0);
	u = inputGen.GetLine(80, 0);
	v = inputGen.GetLine(80, 0);
	rgbASM = inputGen.GetEmptyLine(960);
	rgb = inputGen.GetEmptyLine(960);
	int size=320;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v,160);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 160);
	RecordProperty("CycleCount", cvtColorCycleCount);


	arrCheck.CompareArrays(rgbASM,rgb,960);

	YUVtoRGBcheck(0, 135, 0, size, rgbASM);

	YUVtoRGBcheck(0, 135, 0, size, rgb);


}

TEST_F(cvtColorTest , TestUniformInputLinYUVtoRGBMaxInput)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(48, 255);
	u = inputGen.GetLine(12, 255);
	v = inputGen.GetLine(12, 255);
	rgbASM = inputGen.GetEmptyLine(144);
	rgb = inputGen.GetEmptyLine(144);
	int size=48;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 24);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 24);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,144);

	YUVtoRGBcheck(255, 120, 255, size, rgbASM);

	YUVtoRGBcheck(255, 120, 255, size, rgb);


}

TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVMaxInputMaxY)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetEmptyLine(64);
	u = inputGen.GetEmptyLine(16);
	v = inputGen.GetEmptyLine(16);
	yASM = inputGen.GetEmptyLine(64);
	uASM = inputGen.GetEmptyLine(16);
	vASM = inputGen.GetEmptyLine(16);
	rgb = inputGen.GetLine(192,255);
	int size=64;

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 32);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 32);
	RecordProperty("CycleCount", cvtColorCycleCount);

	RGBtoYUVcheck(yASM,uASM,vASM,64,255,128,128);

	RGBtoYUVcheck(y,u,v,64,255,128,128);
	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);

}

TEST_F(cvtColorTest , TestUniformInputLinYUVtoRGBRandomInput)
{
	y = inputGen.GetEmptyLine(64);
	u = inputGen.GetEmptyLine(16);
	v = inputGen.GetEmptyLine(16);
	rgb = inputGen.GetEmptyLine(192);
	int i,size=64;

	for(i=0;i<size;i++)
	{
		y[i]=dataGenerator.GenerateUInt(0,255,1);
	}
	for(i=0;i<size/4;i++)
	{
		u[i]=dataGenerator.GenerateUInt(0,255,1);
	}
	for(i=0;i<size/4;i++)
	{
		v[i]=dataGenerator.GenerateUInt(0,255,1);
	}

	rgbASM = inputGen.GetEmptyLine(192);
	rgb = inputGen.GetEmptyLine(192);

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 32);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 32);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,192);
}


TEST_F(cvtColorTest , TestUniformInputLineRGBtoYUVRandomInput)
{

	y = inputGen.GetEmptyLine(64);
	u = inputGen.GetEmptyLine(16);
	v = inputGen.GetEmptyLine(16);

	yASM = inputGen.GetEmptyLine(64);
	uASM = inputGen.GetEmptyLine(16);
	vASM = inputGen.GetEmptyLine(16);

	rgb = inputGen.GetEmptyLine(192);

	int i,size=64;

	for(i=0;i<size*3;i++)
	{
		rgb[i]=dataGenerator.GenerateUInt(0,255,1);
	}

	RGBtoYUV_asm_test(rgb,  yASM,  uASM,  vASM, 32);
	cvtColorKernelRGBToYUV(rgb,  y,  u,  v, 32);

	DeltaCheck(y,yASM,size);
	DeltaCheck(u,uASM,size/4);
	DeltaCheck(v,vASM,size/4);
}

TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBMaxR)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(8,122);
	u = inputGen.GetLine(2,178);
	v = inputGen.GetLine(2,241);
	rgbASM = inputGen.GetEmptyLine(24);
	rgb = inputGen.GetEmptyLine(24);
	int size=8;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 4);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 4);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,24);
	YUVtoRGBcheck(255, 23, 210, 8, rgb);
	YUVtoRGBcheck(255, 23, 210, 8, rgbASM);
}


TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBMaxG)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(80,247);
	u = inputGen.GetLine(20,32);
	v = inputGen.GetLine(20,39);
	rgbASM = inputGen.GetEmptyLine(240);
	rgb = inputGen.GetEmptyLine(240);
	int size=80;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 40);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 40);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,240);
	YUVtoRGBcheck(121, 255, 76, 80, rgb);
	YUVtoRGBcheck(121, 255, 76, 80, rgbASM);
}

TEST_F(cvtColorTest , TestUniformInputLineYUVtoRGBMaxB)
{
	inputGen.SelectGenerator("uniform");
	y = inputGen.GetLine(640,245);
	u = inputGen.GetLine(160,197);
	v = inputGen.GetLine(160,100);
	rgbASM = inputGen.GetEmptyLine(1920);
	rgb = inputGen.GetEmptyLine(1920);
	int size=640;

	YUVtoRGB_asm_test(rgbASM,  y,  u,  v, 320);
	cvtColorKernelYUVToRGB(y,	u,	v, rgb, 320);
	RecordProperty("CycleCount", cvtColorCycleCount);

	DeltaCheck(rgb,rgbASM,1920);
	YUVtoRGBcheck(205, 241, 255, 640, rgb);
	YUVtoRGBcheck(205, 241, 255, 640, rgbASM);
}
