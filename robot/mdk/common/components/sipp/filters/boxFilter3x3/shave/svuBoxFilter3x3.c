#include <sipp.h>
#include <sippMacros.h>
#include <filters/boxFilter3x3/boxFilter3x3.h>


/// boxfilter kernel that makes average on 3x3 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers for output lines    
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter3x3(UInt8** in, UInt8** out, UInt32 normalize, UInt32 width)
{

    int i,x,y;
    UInt8* lines[5];
    unsigned int sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0] = *in;
    lines[1] = *(in+1);
    lines[2] = *(in+2);

    //Go on the whole line
    for (i = 0; i < width; i++)
    {
        sum = 0;
        for (y = 0; y < 3; y++)
        {
            for (x = -1; x <= 1; x++)
            {
                sum += (lines[y][x]);
            }
            lines[y]++;
        }
        
        if(normalize == 1)
        {
            *(*out+i)=(UInt8)(((half)(float)sum)*(half)0.1111);
        }
        else 
        {
            *(aux+i)=(unsigned short)sum;
        }
    }
    return;
}

void svuBoxFilter3x3(SippFilter *fptr, int svuNo, int runNo)
{

    UInt8 *output;
    UInt8 *iline[3];
    BoxFilter3x3Param *param = (BoxFilter3x3Param*)fptr->params;
    

    //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    
	output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
  //the output line
#ifdef SIPP_USE_MVCV
    boxfilter3x3_asm(iline, &output, param->normalize, fptr->sliceWidth); 
#else
    boxfilter3x3(iline, &output, param->normalize, fptr->sliceWidth);
#endif
}



