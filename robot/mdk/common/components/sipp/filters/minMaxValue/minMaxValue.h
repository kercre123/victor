#include <sipp.h>

typedef struct
{
	UInt8 minVal;
	UInt8 maxVal;
    UInt8 maskAddr[320];
		
}
minMaxValParam;


//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuMinMaxValue)(SippFilter *fptr, int svuNo, int runNo);