#include <sipp.h>
#include <sippMacros.h>
#include <filters/cvtColorNV21toRGB/cvtColorNV21toRGB.h>

void cvtColorNV21toRGBImplementation(UInt8** yin, UInt8** uvin, UInt8** outR, UInt8** outG, UInt8** outB, UInt32 width) {
	UInt32 i;
	
	UInt8* y  = yin[0];
	UInt8* uv = uvin[0];
	UInt8* rr = outR[0];
	UInt8* gg = outG[0];
	UInt8* bb = outB[0];
	
	UInt32 uvidx = 0;
	int yy,u,v,r,g,b;
	for(i = 0; i < width; i+=2 )
	{
		yy = y[i];
		u = uv[uvidx] - 128;
		v = uv[uvidx+1] - 128;
		uvidx += 2;
			r =  yy + (int)(1.402f*v);
			rr[i] = (UInt8) (r > 255 ? 255 : r < 0 ? 0 : r);  
			g =  yy - (int)(0.344f*u + 0.714*v);
			gg[i] = (UInt8) (g > 255 ? 255 : g < 0 ? 0 : g);
			b =  yy + (int)(1.772f*u);
			bb[i] = (UInt8) (b > 255 ? 255 : b < 0 ? 0 : b);
	//----------------------------------------------
		yy = y[i + 1];
		r =  yy + (int)(1.402f*v);
		rr[i + 1] = (UInt8) (r > 255 ? 255 : r < 0 ? 0 : r);
		g =  yy - (int)(0.344f*u + 0.7148*v);
		gg[i + 1] = (UInt8) (g > 255 ? 255 : g < 0 ? 0 : g);
		b =  yy + (int)(1.772f*u);
		bb[i + 1] = (UInt8) (b > 255 ? 255 : b < 0 ? 0 : b);
	}
}

void svucvtColorNV21toRGB(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *outputR;
    UInt8 *outputG;
    UInt8 *outputB;

    UInt8 *ilineY;
	UInt8 *ilineUV;
	
  //the 2 input lines
    ilineY  =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    ilineUV =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][0], svuNo);
        
  //the output line
    outputR = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	outputG = outputR + fptr->planeStride;
	outputB = outputG + fptr->planeStride;


#ifdef SIPP_USE_MVCV
    cvtColorNV21toRGB_asm(&ilineY, &ilineUV, &outputR, &outputG, &outputB, fptr->sliceWidth); 
#else
    cvtColorNV21toRGBImplementation(&ilineY, &ilineUV, &outputR, &outputG, &outputB, fptr->sliceWidth);
#endif
}
