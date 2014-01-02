#include <sipp.h>
#include <sippMacros.h>
#include <filters/channelExtract/channelExtract.h>

void channelExtract(UInt8** in, UInt8** out, UInt32 width, UInt32 plane)
{
	UInt8 *in_line;
	UInt8 *out_line;
	UInt32 i, c=0;
	in_line  = *in;
	out_line = *out;

	for (i=plane;i<width;i=i+3)
	{
		out_line[c]=in_line[i];
		c++;
	}
    return;
}
void svuChannelExtract(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *input[1];		
    UInt32 i;
	
	//the input line
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	
    //the output line

    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV 	

			channelExtract_asm(input, &output, (fptr->sliceWidth), 0);
			
#else	
		
			channelExtract(input, &output, (fptr->sliceWidth), 0);
		
#endif
}