#include <sipp.h>

typedef struct
{
    UInt32 normalize;
}
BoxFilter7x7Param;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuBoxFilter7x7)(SippFilter *fptr, int svuNo, int runNo);
