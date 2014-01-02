#include <sipp.h>

typedef struct
{
	float threshold1;
	float threshold2;

}
cannyEdgeDetectionParam;




//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuCannyEdgeDetection)(SippFilter *fptr, int svuNo, int runNo);