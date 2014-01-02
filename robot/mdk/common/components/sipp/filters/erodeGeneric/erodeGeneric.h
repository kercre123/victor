#include <sipp.h>

typedef struct
{
    UInt32* eMat;
	UInt8 filterSize;
}
ErodeGenericParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuErodeGeneric)(SippFilter *fptr, int svuNo, int runNo);