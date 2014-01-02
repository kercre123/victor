#include <sipp.h>

typedef struct
{
    UInt16 cMat[3*3]; //fp16 3x3 matrix
}
Conv3x3Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConv3x3)(SippFilter *fptr, int svuNo, int runNo);
