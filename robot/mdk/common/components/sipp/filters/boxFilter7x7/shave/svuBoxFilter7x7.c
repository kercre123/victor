#include <sipp.h>
#include <sippMacros.h>
#include <filters/boxFilter7x7/boxFilter7x7.h>

void boxfilter7x7(UInt8** in, UInt8** out, UInt32 normalize, UInt32 width)
{
    int i,x,y;
    UInt8* lines[7];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<7;x++){
            for (y=0;y<7;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(UInt8)(sum/49.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

void svuBoxFilter7x7(SippFilter *fptr, int svuNo, int runNo)
{
	UInt8 *output;
	UInt8 *iline[7];
    BoxFilter7x7Param *param = (BoxFilter7x7Param*)fptr->params;
    

    //the  input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    iline[3]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
    iline[4]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
	iline[5]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][5], svuNo);
    iline[6]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][6], svuNo);
	
  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    boxfilter7x7_asm(iline, &output, param->normalize, fptr->sliceWidth); 
#else
    
	boxfilter7x7(iline, &output, param->normalize, fptr->sliceWidth);

  #endif
}



