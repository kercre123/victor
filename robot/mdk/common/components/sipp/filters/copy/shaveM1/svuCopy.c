#include <sipp.h>

//##########################################################################################
void svuCopy(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline;
    UInt32 i;

  //the input line
    iline =  (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    
  //the output line
    output = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

  
  //copy-paste
    for(i=0; i<fptr->sliceWidth; i++)
      output[i] = iline[i];
}
