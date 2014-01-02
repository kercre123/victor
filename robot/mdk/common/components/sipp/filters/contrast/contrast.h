#include <sipp.h>

typedef struct
{
  //Params derived by Leon (using float here as Leon doesn't know half)
    float idxLow;
    float scale;
}
ContrastParam;

void SVU_SYM(svuContrast)(SippFilter *fptr, int svuNo, int runNo); 
