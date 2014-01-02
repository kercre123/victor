#include <sipp.h>
/*
typedef struct
{
    UInt32* width;
}
CvtColorYUV422ToRGBParam;*/

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuCvtColorYUV422ToRGB)(SippFilter *fptr, int svuNo, int runNo);