#include <sipp.h>

typedef struct
{
	
		float alpha;
	
}
AccumulateWeightedParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuAccumulateWeighted)(SippFilter *fptr, int svuNo, int runNo);
