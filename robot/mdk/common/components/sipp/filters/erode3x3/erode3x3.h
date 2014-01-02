#include <sipp.h>

typedef struct
{
    UInt32* eMat;
}
Erode3x3Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuErode3x3)(SippFilter *fptr, int svuNo, int runNo);