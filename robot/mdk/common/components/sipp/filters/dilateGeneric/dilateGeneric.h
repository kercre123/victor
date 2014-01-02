#include <sipp.h>

typedef struct
{
    UInt32 *dMat;
	UInt32 kernelSize;
}
DilateGenericParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuDilateGeneric)(SippFilter *fptr, int svuNo, int runNo);
