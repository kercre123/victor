#include <sipp.h>
#include <sippMacros.h>
#include <filters/threshold/threshold.h>


void thresholdKernel(UInt8** in, UInt8** out, UInt32 width, UInt32 height, UInt8 thresh, UInt32 thresh_type)

{
    UInt32 i, j;
    UInt8* in_1;

    in_1 = *in;


    UInt8 max_value = 0xff;

    for(i = 0; i < height ; i++)
    {
        for(j = 0; j < width; j++)
        {
            switch(thresh_type)
            {
                case Thresh_To_Zero:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    else
                    {
                        out[i][j] = 0;
                    }
                    break;
                case Thresh_To_Zero_Inv:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = 0;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
                case Thresh_To_Binary:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = max_value;
                    }
                    else
                    {
                        out[i][j] = 0;
                    }
                    break;
                case Thresh_To_Binary_Inv:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = 0;
                    }
                    else
                    {
                        out[i][j] = max_value;
                    }
                    break;
                case Thresh_Trunc:
                    if((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = thresh;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
                default:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = thresh;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
            }
        }
    }

    return;
}

void svuThreshold(SippFilter *fptr, int svuNo, int runNo)
{

    UInt8 *output;
    UInt8 *iline[1];
    ThresholdParam *param = (ThresholdParam*)fptr->params;
    

    //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    

  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    thresholdKernel_asm(iline, &output, fptr->sliceWidth, 1, param->thresholdValue, param->threshType); 
#else
    
	thresholdKernel(iline, &output, fptr->sliceWidth, 1, param->thresholdValue, param->threshType); 

  #endif
}


