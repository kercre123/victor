#include <sipp.h>
#include <sippMacros.h>
#include <filters/genDnsRefP/genDnsRefP.h>

//##########################################################################################
void svuGenDnsRefP(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 x;
    int    xc; //centered Squared X coord (center relative)
    int    yc; //centered Squared Y coord (center relative)
    float  gain;

    YDnsRefPParam *param = (YDnsRefPParam *)fptr->params;

  //Input Y line
    UInt8 *inY  = (UInt8*)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo);

  //Output Y ref
    UInt8 *outY = (UInt8*)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);

  //Y coord is constant for entire line, so can compute here.
    yc = (runNo - fptr->outputH/2);
    yc = yc * yc;

    for(x=0; x<fptr->sliceWidth; x++)
    {
        xc = (((svuNo-fptr->gi->sliceFirst) * fptr->sliceWidth) + x) - fptr->outputW/2;
        xc = xc * xc;
        gain = (float) param->lut[(xc+yc) >> param->shift] / 256.0f;
        outY[x] = inY[x] * gain;
    }
} 
