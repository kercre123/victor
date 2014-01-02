#include <sipp.h>

typedef struct
{
    UInt32 *sumVect;
}
IntegralImageSqSumU32Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuIntegralImageSqSumU32)(SippFilter *fptr, int svuNo, int runNo);
