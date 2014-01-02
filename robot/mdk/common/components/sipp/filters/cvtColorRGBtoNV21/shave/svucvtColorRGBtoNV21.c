#include <sipp.h>

void cvtColorKernelRGBToLuma(UInt8** inR, UInt8** inG, UInt8** inB, UInt8** yOut, UInt32 width) 
{
	UInt32 i;
	
	UInt8* r = inR[0];
	UInt8* g = inG[0];
	UInt8* b = inB[0];
	UInt8* yo = yOut[0];
	
	int y;
	
	for (i = 0; i < width; i++)
    {
        y = 0.299f * r[i] + 0.587f * g[i] + 0.114f * b[i];
        yo[i] = (UInt8) y > 255 ? 255 : y < 0 ? 0 : y;
    }
}
//#######################################################################################
void svuCvtColorRGBToLuma(SippFilter *fptr, int svuNo, int runNo)
{
    	
    UInt8 *inputR;	
    UInt8 *inputG;	
    UInt8 *inputB;	

    UInt8 *output;
	
	//the 3 input lines
	inputR =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	inputG = inputR + fptr->parents[0]->planeStride;
	inputB = inputG + fptr->parents[0]->planeStride;	
		
    //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
 
	cvtColorKernelRGBToLuma(&inputR, &inputG, &inputB, &output, fptr->sliceWidth);
}

void cvtColorKernelRGBToUV(UInt8** inR, UInt8** inG, UInt8** inB, UInt8** uvOut, UInt32 width) 
{
	UInt32 i,j;
	
	UInt8* r = inR[0];
	UInt8* g = inG[0];
	UInt8* b = inB[0];
	UInt8* uvo = uvOut[0];
	
	int y,u1, u2, v1, v2, um, vm;
	UInt32 uv_idx = 0;
	
	for (i = 0; i < width; i+=2)
    {
        
        y = 0.299f * r[i] + 0.587f * g[i] + 0.114f * b[i];
        //yo[i] = (UInt8) y > 255 ? 255 : y < 0 ? 0 : y;
		
		if (i % 2 == 0) {
           	u1 = (int)(((float)b[i] - y) * 0.564f) + 128;
        	v1 = (int)(((float)r[i] - y) * 0.713f) + 128;
        }
	//-------------------------------------------------------
		
		y = 0.299f * r[i+1] + 0.587f * g[i+1] + 0.114f * b[i+1];
		//yo[i + 1] = (UInt8) y > 255 ? 255 : y < 0 ? 0 : y;
					
		if (i % 2 == 0) {
			u2 = (int)(((float)b[i+1] - y) * 0.564f) + 128;
			v2 = (int)(((float)r[i+1] - y) * 0.713f) + 128;
        
		um = (u1 + u2)/2;
        vm = (v1 + v2)/2;
        uvo[uv_idx] = (UInt8) um > 255 ? 255 : um < 0 ? 0 : um;
		uvo[uv_idx + 1] = (UInt8) vm > 255 ? 255 : vm < 0 ? 0 : vm;
		uv_idx = uv_idx + 2;
		}
		
 	}
	
}

//#######################################################################################
void svuCvtColorRGBToUV(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *inputR;	
    UInt8 *inputG;	
    UInt8 *inputB;	

    UInt8 *output;
	
	//the 3 input lines
	inputR =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	inputG = inputR + fptr->parents[0]->planeStride;
	inputB = inputG + fptr->parents[0]->planeStride;	
		
    //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
 
	cvtColorKernelRGBToUV(&inputR, &inputG, &inputB, &output, fptr->sliceWidth);
}