#include <sipp.h>

//http://en.wikipedia.org/wiki/Distortion_%28optics%29

typedef struct
{
    int   cx, cy;  //Distortion center
    float p1, p2;  //Tangential distortion coefs
    float k1, k2;  //Radial distortion coefficient,
}
UndistortBParam;

void SVU_SYM(svuUndistortBrown)(SippFilter *fptr, int svuNo, int runNo); 
