#include <sipp.h>

typedef struct
{
    
	UInt16 lutValue[1024];
}
Lut10to16Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuLut10to16)(SippFilter *fptr, int svuNo, int runNo);
