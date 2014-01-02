#include <sipp.h>


//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuMedianFilter7x7)(SippFilter *fptr, int svuNo, int runNo);
