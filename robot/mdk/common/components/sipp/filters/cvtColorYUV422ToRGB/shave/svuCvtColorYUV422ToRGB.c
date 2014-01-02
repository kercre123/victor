#include <sipp.h>
#include <sippMacros.h>
#include <filters/cvtColorYUV422ToRGB/cvtColorYUV422ToRGB.h>


void cvtColorKernelYUV422ToRGB(UInt8** input, UInt8** rOut, UInt8** gOut, UInt8** bOut, UInt32 width)
{	
	int j;
	int r, g, b, y1, y2, u, v;
	int out_index = 0;
	UInt8*  in = input[0];
	UInt8* outR = rOut[0];
	UInt8* outG = gOut[0];
	UInt8* outB = bOut[0];
	
	for(j = 0; j < width * 2; j+=4)
	{
		y1 = in[j];
		u  = in[j + 1] - 128;
		y2 = in[j + 2];
		v  = in[j + 3] - 128;
		
		r = y1 + (int)(1.402f * v);
        g = y1 - (int)(0.344f * u + 0.714f * v);
        b = y1 + (int)(1.772f * u);
		
		outR[out_index] = (UInt8) (r>255 ? 255 : r<0 ? 0 : r);
		outG[out_index] = (UInt8) (g>255 ? 255 : g<0 ? 0 : g);
		outB[out_index] = (UInt8) (b>255 ? 255 : b<0 ? 0 : b);
		out_index++;
	
		r = y2 + (int)(1.402f * v);
        g = y2 - (int)(0.344f * u + 0.714f * v);
        b = y2 + (int)(1.772f * u);

		outR[out_index] = (UInt8) (r>255 ? 255 : r<0 ? 0 : r);
		outG[out_index] = (UInt8) (g>255 ? 255 : g<0 ? 0 : g);
		outB[out_index] = (UInt8) (b>255 ? 255 : b<0 ? 0 : b);
		out_index++;
	}
}


void svuCvtColorYUV422ToRGB(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *outputR;
    UInt8 *outputG;
    UInt8 *outputB;
    UInt8 *input;	
	
	//the input line
	input = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);	
		
    //the 3 output lines	
    outputR = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
    outputG = outputR + fptr->planeStride;
    outputB = outputG + fptr->planeStride;
  
 #ifdef SIPP_USE_MVCV 	
	cvtColorKernelYUV422ToRGB_asm(&input, &outputR, &outputG, &outputB, fptr->sliceWidth);
#else		
	cvtColorKernelYUV422ToRGB(&input, &outputR, &outputG, &outputB, fptr->sliceWidth);
#endif
}