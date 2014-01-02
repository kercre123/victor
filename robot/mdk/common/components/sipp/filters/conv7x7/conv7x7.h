#include <sipp.h>

typedef struct
{
    UInt16* cMat; //fp16 7x7 matrix
}
Conv7x7Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConv7x7)(SippFilter *fptr, int svuNo, int runNo);
