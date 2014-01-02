#include <sipp.h>
#include <sippMacros.h>
#include <filters/homography/homography.h>

//#############################################################################
void svuHomography(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32   x;
    float   *mat3x3 = ((HomographyParam *)fptr->params)->homoMat3x3;

  //I/O pointers
    UInt8 *in;
    UInt8 *out     = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);
    UInt8  interp  = 0;
    int    inLines = fptr->parents[0]->nLines;

    for(x=0; x<fptr->sliceWidth; x++)
    {
      //Figure out the absolute X coordinates (the Y is absolute already)
        float absX = (svuNo - fptr->gi->sliceFirst)*fptr->sliceWidth + x;
        float absY = runNo;

      //Center coordinates:
        absX -= fptr->outputW/2;
        absY -= fptr->outputH/2;

      //Transform the coords according to Homography matrix:
        float u = mat3x3[0]*absX + mat3x3[1]*absY + mat3x3[2];
        float v = mat3x3[3]*absX + mat3x3[4]*absY + mat3x3[5];
        float w = mat3x3[6]*absX + mat3x3[7]*absY + mat3x3[8];

      //Scale u,v by 1/w
        w = 1 / w;
        u = u * w;
        v = v * w;

      //Back to abs coords
        u += fptr->outputW/2;
        v += fptr->outputH/2;

      //Interpolate/sample the data at UV coords
        if ( (u<0)||(u>fptr->outputW) ||
             (v<0)||(v>fptr->outputH) )
        {
          //if U,V are out of image coord space, put some default color
            interp = 0;
        }
        else{//Pixel coords are within valid range, interpolate
             //for now, just use a dummy nearest neighbor implementation
              
             //Here's the trick: we don't assume data is in current slice,
             //so need to compute the absolute address of the pixels of interest
             //which depends on the slices

             //Important, V is an absolute value, but find the v relative to 
             int slice  = ((int)u) / fptr->sliceWidth + fptr->gi->sliceFirst;
             int offset = ((int)u) % fptr->sliceWidth; //offset within the slice
             in = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][inLines/2 + ((int)v - runNo)] , slice);

             //Interpolation: nearest neighbor, could do more complex
             //normally would need to consider fractional parts of u, v here
             //and do bilinear or bicubic interpolation
             interp = in[offset];
        }
       
        out[x] = interp;
    }
} 
