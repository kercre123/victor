#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP403DnsLuma/fP403DnsLuma.h>

#define REF_KER_SZ 11
#define INL_KER_SZ 7
#define INL_KER_RD (INL_KER_SZ>>1)

#define U12F_TO_FP16(x)    fp16((x)*(1.0f/4095))

static inline int MSB(UInt8 a)
{
	int len = sizeof(UInt8)*8;
	uint64_t bit = 1<<(len-1);
	int i = 0;
	for(i=len;i>=0;i--)
		if( (bit & a) )
			break;
		else
			bit = bit>>1;
	if (i<0)
		i=0;
	return i;
}
//##########################################################################################
void svufP403DnsLuma(SippFilter *fptr, int svuNo, int runNo)
{
   const UInt32 KERNEL_SIZE = 11;
   const UInt32 KernelLuma = 7;
    
   const float s1=20;
   const float s2=220;
   const float s3=250;
    
   UInt32 x;
   UInt32 ln;

   half  *in[INL_KER_SZ];
   UInt8 *inRef[REF_KER_SZ];
   half  *out;

   UInt16 denoised = 0;

   for(ln=0; ln<INL_KER_SZ; ln++)
   {
      in[ln]    = (half  *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][ln], svuNo); 
   }
   for(ln=0; ln<REF_KER_SZ; ln++)
   {
      inRef[ln] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][ln], svuNo); 
   }

   //Initially points to CR, will add fptr->planeStride to go to next plane
   out = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);    
    
   FP403DnsLumaParam *param = (FP403DnsLumaParam*)fptr->params;
  
   int yCentery = (int)(KERNEL_SIZE>>1);

   //3x3 blur ... could be optimized...
   for(x=0; x<fptr->sliceWidth; x++)
   {
      UInt32 wt = 0;
      UInt32 accum = 0;
        
      for(int shifty = -3 ; shifty <=3 ;shifty++)
      {
         for (int shiftx = -3 ; shiftx <=3 ;shiftx++)
         {
            // box filter result
            UInt32 diff = 0;
            // sad for 5x5 kernel
            for (int ySad = -2; ySad <= 2; ySad++)
            {
               for (int xSad = -2; xSad <= 2; xSad++)
               {                
                  UInt8 difp8 = abs((int)inRef[yCentery + ySad][x + xSad] - (int)inRef[yCentery + shifty + ySad][x + shiftx + xSad]);
                  UInt16 difp = difp8;
                  difp <<= (MSB(difp8)-1);
                  diff += difp;            
               }
            }

            UInt32 dpoint = diff;
            dpoint = dpoint > 0xffff ? 0xffff : dpoint;
            dpoint >>= param->strength;
            dpoint  = dpoint > 31 ? 31 : dpoint;

            int kx = (shiftx+3);
            int ky = shifty +3;
            int kidx = ky*7 + kx;
            UInt16 wt_tmp = param->lut[dpoint] * param->f2[kidx];
            wt += wt_tmp;
            
            float aa = (float)in[INL_KER_RD + shifty][x + shiftx];
            accum += (UInt32)wt_tmp * (UInt16)(in[INL_KER_RD + shifty][x + shiftx] * 4095 + 0.5f);
            printf("");
         }
      }        
        
      uint16_t weight = wt;
      weight = weight == 0 ? 1 : weight;
      denoised = (uint16_t)(accum / wt);
      out[x] = U12F_TO_FP16(denoised);
   }
} 
