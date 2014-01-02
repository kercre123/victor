#include <sipp.h>
#include <sippMacros.h>
#include <filters/boxFilter/boxFilter.h>

void boxfilter(UInt8** in, UInt8** out, UInt32 k_size, UInt32 normalize, UInt32 width)
{
    int i,x,y;
    UInt8* lines[15];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //initialize line pointers
    for(i=0;i<k_size;i++)
    {
        lines[i]=*(in+i);
    }

    //compute the actual out pixel
    for (i=0;i<width;i++)
    {
        sum=0.0f;
        for (x=0;x<k_size;x++)
        {
            for (y=0;y<k_size;y++)
            {
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(UInt8)(sum/((float)k_size*k_size));
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}


void svuBoxFilter(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[15];
    BoxFilterParam *param = (BoxFilterParam*)fptr->params;
    UInt32 i;

    
    for (i = 0; i < param->filterSize; i++)
		iline[i] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][i], svuNo);
    

  //the output line

    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    boxfilter_asm(iline, &output, param->filterSize, param->normalize, fptr->sliceWidth); 

#else

	boxfilter(iline, &output, param->filterSize, param->normalize, fptr->sliceWidth);
	
	
#endif
}
  



