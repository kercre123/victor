#include <sipp.h>
#include <sippMacros.h>
#include <filters/chromaBlock/chromaBlock.h>

//##########################################################################################
void svuChromaBlock(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32         x;
    float          cR, cG, cB;    //desaturated chromas R,G,B
    float          r,  g,  b;    //Converted R,G,B
    ChromaBlkParam *param = (ChromaBlkParam *)fptr->params;
    float          *ccm   = param->ccm;
    UInt8          *lut   = param->rangeLut;
    float          alpha, cavg;

  //Input pointers
    UInt8  *inCR = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    UInt8  *inCG = inCR + fptr->parents[0]->planeStride;
    UInt8  *inCB = inCG + fptr->parents[0]->planeStride;
    half   *inY  = (half  *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][0], svuNo);
  //Output pointers
    half   *outR = (half *)WRESOLVE(fptr->dbLineOut   [runNo&1]   , svuNo);
    half   *outG = outR + fptr->planeStride; 
    half   *outB = outG + fptr->planeStride; 


    for(x=0; x<fptr->sliceWidth; x++)
    {
      //Chroma desaturation
        alpha  = lut[clampU8(inY[x]*255.0f)] * (1.0f/255.0f);
        cavg   = ((float)inCR[x] + (float)inCG[x] + (float)inCB[x]) * (1.0f/3.0f);
        cR   = (inCR[x] * alpha + cavg *(1-alpha));
        cG   = (inCG[x] * alpha + cavg *(1-alpha));
        cB   = (inCB[x] * alpha + cavg *(1-alpha));

      //Luma, Chroma combination => RGB
        r    = (cR * inY[x] * 3.0f)/255.0f;
        g    = (cG * inY[x] * 3.0f)/255.0f; 
        b    = (cB * inY[x] * 3.0f)/255.0f; 

      //Color Correction & Saturation Matrix
        outR[x] = clamp01(r * ccm[0]  +  g * ccm[1]  +  b * ccm[2]);
        outG[x] = clamp01(r * ccm[3]  +  g * ccm[4]  +  b * ccm[5]);
        outB[x] = clamp01(r * ccm[6]  +  g * ccm[7]  +  b * ccm[8]);
    }
}
