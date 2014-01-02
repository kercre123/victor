#include <sipp.h>

typedef struct
{
    float *homoMat3x3; //Ptr to 3x3 Homography matrix
}
HomographyParam;

void SVU_SYM(svuHomography)(SippFilter *fptr, int svuNo, int runNo); 
