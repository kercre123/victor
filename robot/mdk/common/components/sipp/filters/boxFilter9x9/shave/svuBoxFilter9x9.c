#include <sipp.h>
#include <sippMacros.h>
#include <filters/boxFilter9x9/boxFilter9x9.h>

void boxfilter9x9(UInt8** in, UInt8** out, UInt32 normalize, UInt32 width)
{
    int i,x,y;
    UInt8* lines[9];
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
    lines[7]=*(in+7);
    lines[8]=*(in+8);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<9;x++){
            for (y=0;y<9;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(UInt8)(sum/81.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

void svuBoxFilter9x9(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[9];
    BoxFilter9x9Param *param = (BoxFilter9x9Param*)fptr->params;
    

    //the  input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    iline[3]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
    iline[4]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
	iline[5]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][5], svuNo);
    iline[6]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][6], svuNo);
	iline[7]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][7], svuNo);
    iline[8]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][8], svuNo);
	
    //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);


  

#ifdef SIPP_USE_MVCV
    boxfilter9x9_asm(iline, &output, param->normalize, fptr->sliceWidth); 
#else
    
	boxfilter9x9(iline, &output, param->normalize, fptr->sliceWidth);

  #endif
}



