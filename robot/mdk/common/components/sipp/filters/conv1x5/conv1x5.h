#include <sipp.h>

typedef struct
{
#ifdef SIPP_USE_MVCV
    UInt32* cMat; 
#else
    float* cMat;
#endif
}
Conv1x5Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConv1x5)(SippFilter *fptr, int svuNo, int runNo);
