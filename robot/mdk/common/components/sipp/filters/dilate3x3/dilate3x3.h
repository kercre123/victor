#include <sipp.h>

typedef struct
{
     UInt8 (*dMat)[3][3]; 
} Dilate3x3Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuDilate3x3)(SippFilter *fptr, int svuNo, int runNo);
