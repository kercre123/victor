#include <sipp.h>

typedef struct
{
    UInt8 lutValue[256];
}
Lut8to8Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuLut8to8)(SippFilter *fptr, int svuNo, int runNo);
