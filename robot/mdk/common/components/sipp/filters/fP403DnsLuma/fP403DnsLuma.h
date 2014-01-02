#include <sipp.h>

typedef struct
{
    UInt8  *lut;
    UInt32  *f2;
    UInt32  strength;
}FP403DnsLumaParam;

//uint16_t lut[32] = {
//15,    15,    14,    13,    
//12,    11,     9,     8,     
//7,     5,     4,     3,     
//2,     2,     1,     1,     
//1,     0,     0,     0,     
//0,     0,     0,     0,     
//0,     0,     0,     0,     
//0,     0,     0,     0 
//};


//	int F2[]  = {
//		1, 1 ,2 ,2, 2, 1 ,1,
//		1 ,2, 2, 4, 2, 2, 1,
//		2 ,2 ,4, 4, 4, 2, 2,
//		2, 4, 4, 4, 4, 4, 2,
//		2, 2, 4, 4, 4, 2, 2,
//		1, 2, 2, 4, 2, 2, 1,
//		1, 1, 2, 2, 2, 1, 1,
//	};

void SVU_SYM(svufP403DnsLuma)(SippFilter *fptr, int svuNo, int runNo); 
