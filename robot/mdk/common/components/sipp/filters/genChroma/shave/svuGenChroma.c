#include <sipp.h>
#include <sippMacros.h>
#include <filters/genChroma/genChroma.h>

//#define CHROMA_EPSILON (1.0f/1024.0f)

//Octave ref
//	oo_luma = 1 ./ (presharp + cfg.chroma_epsilon);
//	for i = 1:3
//		ci(:,:,i) = lanczos_downsample(img(:,:,i) .* oo_luma * (255/3));
//	end

//extern void subs_05_sync7(SippFilter *fptr, half *iline[7], half *oline, int runNo);

//##########################################################################################
void svuGenChroma(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;
    UInt32 ooLuma;
    float  eps   = ((ChrGenParam*)fptr->params)->epsilon;

  //Input RGB lines (parent[0] must be the 3xplane RGB)
    UInt8 *inR   = (UInt8 *)getInPtr(fptr, 0, 0, 0); 
    UInt8 *inG   = (UInt8 *)getInPtr(fptr, 0, 0, 1); 
    UInt8 *inB   = (UInt8 *)getInPtr(fptr, 0, 0, 2); 

  //Input second parent
    UInt8 *inY   = (UInt8 *)getInPtr(fptr, 1, 0, 0); 

  //Output lines
    UInt8  *outCR = (UInt8 *)getOutPtr(fptr, 0);
    UInt8  *outCG = outCR + fptr->planeStride;
    UInt8  *outCB = outCG + fptr->planeStride;

  //The actual processing
    for(i=0; i<fptr->sliceWidth; i++)
    {  //div oo_luma by extra amount, to scale from input #bits to 8bit output
        ooLuma   =  (inY[i] + eps);
        outCR[i] =  clampU8(inR[i] * (255/3) / ooLuma);
        outCG[i] =  clampU8(inG[i] * (255/3) / ooLuma);
        outCB[i] =  clampU8(inB[i] * (255/3) / ooLuma);
    }
} 
