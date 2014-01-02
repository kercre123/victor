#include <sipp.h>

typedef struct
{
    
	UInt8 thresholdValue;
	UInt32 threshType;
	
}
ThresholdParam;
enum
{
    Thresh_To_Zero       = 0,
    Thresh_To_Zero_Inv   = 1,
    Thresh_To_Binary     = 2,
    Thresh_To_Binary_Inv = 3,
    Thresh_Trunc         = 4
    //THRESH_MASK        = CV_THRESH_MASK,
    //THRESH_OTSU        = CV_THRESH_OTSU
};

//Shave symbols that need to be understood by leon need to be declared through "SVU_SYM" MACRO,
//as moviCompile adds a leading _ to symbol exported
void SVU_SYM(svuThreshold)(SippFilter *fptr, int svuNo, int runNo);
