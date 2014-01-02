#include <sipp.h>

typedef struct
{
    int maxVal; // fp16 will be in interval 0-1, for make conversion need max val for specifien bpp
    int channelPlace;
}FP400ExtractLumaParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svufP400ExtractLuma)(SippFilter *fptr, int svuNo, int runNo);
