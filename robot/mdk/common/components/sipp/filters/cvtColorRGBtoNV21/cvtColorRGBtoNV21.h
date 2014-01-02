#include <sipp.h>

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuCvtColorRGBToLuma)(SippFilter *fptr, int svuNo, int runNo);
void SVU_SYM(svuCvtColorRGBToUV  )(SippFilter *fptr, int svuNo, int runNo);