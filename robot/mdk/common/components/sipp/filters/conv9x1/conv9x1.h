#include <sipp.h>

typedef struct
{
    UInt32* cMat;
}
Conv9x1Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConv9x1)(SippFilter *fptr, int svuNo, int runNo);
