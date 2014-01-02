#include <sipp.h>
#include <sippMacros.h>

//##########################################################################################
void svuLumaBlur(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 x;

  //I/O pointers
    UInt8 *outY = (UInt8*)getOutPtr(fptr, 0);
    UInt8 *inY[3];
    
    inY[0] = (UInt8*)getInPtr(fptr, 0, 0, 0);
    inY[1] = (UInt8*)getInPtr(fptr, 0, 1, 0);
    inY[2] = (UInt8*)getInPtr(fptr, 0, 2, 0);
    
    //3x3 blur 
    for(x=0; x<fptr->sliceWidth; x++)
    {  //div by 16 as sum of kernels = 16
        outY[x] = clampU8((inY[0][x-1]*1 + inY[0][x]*2 + inY[0][x+1]*1+
                           inY[1][x-1]*2 + inY[1][x]*4 + inY[1][x+1]*2+
                           inY[2][x-1]*1 + inY[2][x]*2 + inY[2][x+1]*1)>>4);
         
    }
}
