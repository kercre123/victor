#include <sipp.h>

typedef struct
{
      
	  UInt32 *cum_hist;

}
EqualizeHistParam;

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuEqualizeHist)(SippFilter *fptr, int svuNo, int runNo);