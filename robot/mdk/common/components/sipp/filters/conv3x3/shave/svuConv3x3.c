#include <sipp.h>
#include <sippMacros.h>
#include <filters/conv3x3/conv3x3.h>

//##########################################################################################
void conv3x3FilterImplementation (UInt8 *inLine[3], UInt8 *outLine, half *f, UInt32 widthLine)
{
    UInt32 x;
     
    for (x=0; x<widthLine; x++)
    {
        float v00 = (float)inLine[0][x-1];
        float v01 = (float)inLine[0][x  ];
        float v02 = (float)inLine[0][x+1];
                                   
        float v10 = (float)inLine[1][x-1];
        float v11 = (float)inLine[1][x  ];
        float v12 = (float)inLine[1][x+1];
                                   
        float v20 = (float)inLine[2][x-1];
        float v21 = (float)inLine[2][x  ];
        float v22 = (float)inLine[2][x+1];


        float vout =  f[0]*v00 + f[1]*v01 + f[2]*v02 + 
                      f[3]*v10 + f[4]*v11 + f[5]*v12 +  
                      f[6]*v20 + f[7]*v21 + f[8]*v22;

        outLine[x] = clampU8(vout);
    }
}

//##########################################################################################
void svuConv3x3(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[3];
    Conv3x3Param *param = (Conv3x3Param*)fptr->params;
    UInt32 i;

  //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    
  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

  
  #ifdef SIPP_USE_MVCV
    Convolution3x3_asm(iline, &output, (half*)param->cMat, fptr->sliceWidth);
  #else
    conv3x3FilterImplementation(iline, output, (half*)param->cMat, fptr->sliceWidth);
  #endif
}
