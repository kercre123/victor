#include <sipp.h>

typedef struct
{
	UInt8 maskAddr[320];
	UInt8 pixelValue;
	UInt32 pixelPosition;
	UInt8 status;
	
}
positionKernelParam;


//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuPositionKernel)(SippFilter *fptr, int svuNo, int runNo);