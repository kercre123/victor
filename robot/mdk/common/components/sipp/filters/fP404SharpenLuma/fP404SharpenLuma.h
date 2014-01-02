#include <sipp.h>

typedef struct
{
    float strength; //!!!3.00000000
    float thresh; // (thresh/255.0f)*maxval; uint16_t maxval = ((uint32_t)1<<(BPP))-1; bpp = 10; thresh = 2; !!! 8.02352905
    float overshoot; // 1+limit/100.f; limit = 0.0; !!!1.00000000
    float clipping; //0.000000
    float *coef;
    float *s;
}FP404SharpenLumaParam;

void SVU_SYM(svufP404SharpenLuma)(SippFilter *fptr, int svuNo, int runNo); 
