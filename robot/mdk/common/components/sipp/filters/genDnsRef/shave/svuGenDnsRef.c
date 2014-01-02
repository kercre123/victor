#include <sipp.h>
#include <sippMacros.h>
#include <filters/genDnsRef/genDnsRef.h>

//##########################################################################################
void svuGenDnsRef(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 x;
    int    x0;
    int    xc; //centered Squared X coord (center relative)
    int    yc; //centered Squared Y coord (center relative)

    YDnsRefParam *param = (YDnsRefParam *)fptr->params;

  //Input Y line
    UInt8 *inY = (UInt8 *)getInPtr(fptr, 0, 0, 0);

  //Output Y ref
    UInt8  *outY = (UInt8 *)getOutPtr(fptr, 0);

  //Y coord is constant for entire line, so can compute here.
    yc = (runNo - fptr->outputH/2);
    yc = yc * yc;

    x0 = ((svuNo-fptr->gi->sliceFirst) * fptr->sliceWidth) - fptr->outputW/2;

    for(x=0; x<fptr->sliceWidth; x++)
    {
        xc = (x0 + x);
        xc = xc * xc;
        outY[x] = ((UInt32)param->lutGamma[inY[x]] * param->lutDist[(xc+yc)>>param->shift])>>8;
    }
} 
