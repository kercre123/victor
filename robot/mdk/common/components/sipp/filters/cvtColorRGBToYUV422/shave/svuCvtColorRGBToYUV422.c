#include <sipp.h>
#include <sippMacros.h>
#include <filters/cvtColorRGBToYUV422/cvtColorRGBToYUV422.h>

void cvtColorKernelRGBToYUV422(UInt8** rIn, UInt8** gIn, UInt8** bIn, UInt8** output, UInt32 width)
{
	int j;
	float y, u1, v1, u2, v2, u_avr, v_avr;
	int out_index = 0;
	UInt8* inR = rIn[0];
	UInt8* inG = gIn[0];
	UInt8* inB = bIn[0];
	UInt8* out_yuyv = output[0];
	
	for(j = 0; j < width; j+=2)
	{
		y = 0.299f * inR[j] + 0.587f * inG[j] + 0.114f * inB[j];
		u1 = ((float)inB[j] - y) * 0.564f + 128;
        v1 = ((float)inR[j] - y) * 0.713f + 128;
		
		y = y>255 ? 255 : y<0 ? 0 : y;
		out_yuyv[out_index++] =(UInt8) y;

		y = 0.299f * inR[j + 1] + 0.587f * inG[j + 1] + 0.114f * inB[j + 1];
		y = y>255 ? 255 : y<0 ? 0 : y;
		u2 = ((float)inB[j + 1] - y) * 0.564f + 128;
        v2 = ((float)inR[j + 1] - y) * 0.713f + 128;

		u_avr = (u1 + u2) / 2;
		v_avr = (v1 + v2) / 2;
		
		out_yuyv[out_index++] =(UInt8) (u_avr>255 ? 255 : u_avr<0 ? 0 : u_avr);
		out_yuyv[out_index++] =(UInt8) y;	
		out_yuyv[out_index++] =(UInt8) (v_avr>255 ? 255 : v_avr<0 ? 0 : v_avr);		
	}
}


void svuCvtColorRGBToYUV422(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;	
    UInt8 *inputR;	
    UInt8 *inputG;	
    UInt8 *inputB;	
	
	//the 3 input lines
	inputR =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	inputG = inputR + fptr->parents[0]->planeStride;
	inputB = inputG + fptr->parents[0]->planeStride;	
		
    //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
 

#ifdef SIPP_USE_MVCV 	
	cvtColorKernelRGBToYUV422_asm(&inputR, &inputG, &inputB, &output, fptr->sliceWidth);
#else		
	cvtColorKernelRGBToYUV422(&inputR, &inputG, &inputB, &output, fptr->sliceWidth);
#endif
}