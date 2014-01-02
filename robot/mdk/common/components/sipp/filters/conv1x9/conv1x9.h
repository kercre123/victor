#include <sipp.h>

typedef struct
{
#ifdef SIPP_USE_MVCV
    UInt32* cMat; //fp16 5x5 matrix
#else
    float* cMat;
#endif
}
Conv1x9Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConv1x9)(SippFilter *fptr, int svuNo, int runNo);
