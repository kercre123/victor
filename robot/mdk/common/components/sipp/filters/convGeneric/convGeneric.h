#include <sipp.h>

typedef struct
{
    UInt32* cMat; //fp16 matrix
	UInt32 filterSize; //u32 kernel size
}
ConvGenericParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuConvGeneric)(SippFilter *fptr, int svuNo, int runNo);
