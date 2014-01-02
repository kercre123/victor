#include <sipp.h>

typedef struct
{
  UInt32 st_Y;//Starting point on Vertical
  //on horizontal max offset is given by padding param... to deal with this later...
}
CropParam;

void SVU_SYM(svuCrop)(SippFilter *fptr, int svuNo, int runNo); 
