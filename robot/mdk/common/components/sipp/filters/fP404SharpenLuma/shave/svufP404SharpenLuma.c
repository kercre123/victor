#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP404SharpenLuma/fP404SharpenLuma.h>

//##########################################################################################
void svufP404SharpenLuma(SippFilter *fptr, int svuNo, int runNo)
{
   const UInt32 KERNEL_SIZE = 7;
   int x;
   int  ln;
   half *in[KERNEL_SIZE];
   half *out;
   FP404SharpenLumaParam *param = (FP404SharpenLumaParam*)fptr->params;
   float undershoot = 1/param->overshoot;
	// Set rounding mode towards zero to match RTL
   unsigned int rnd;   

   // initialization pointer to in/out lines
   for(ln=0; ln<KERNEL_SIZE; ln++)
   {
      in[ln] = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][ln], svuNo); 
   }
   out = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);    
   // Set rounding mode towards zero to match RTL   
#ifndef WIN32
	rnd = fegetround();
	fesetround(FE_TOWARDZERO);
#else
	_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif

   // Start processing
   for(x = 0; x < (int)fptr->sliceWidth; x++)
   {
      half hp;
      half denoised;
      half lut;
      half wpoint;
      half Wmax;
      half Wmin;
      half delta_max;
      half delta_min;
      half wp;
      half d;
      half Wsmooth;
      half  hzWsmoth[7];
      int ki;

      // vertical gauss
      for (ki = (int)x - (int)3; ki <= ((int)x+(int)3); ki++)
      {
         hzWsmoth[ki + 3 - x] = half(  (double)half((in[0][ki] + in[6][ki]) * (half)param->coef[0]) +
                                       (double)half((in[1][ki] + in[5][ki]) * (half)param->coef[1]) +
                                       (double)half((in[2][ki] + in[4][ki]) * (half)param->coef[2]) +
                                       (double)half((in[3][ki]            ) * (half)param->coef[3])) ;
      }
      // vertical gauss
      Wsmooth = half((double)half((hzWsmoth[0] + hzWsmoth[6]) * (half)param->coef[0]) +
                     (double)half((hzWsmoth[1] + hzWsmoth[5]) * (half)param->coef[1]) +
                     (double)half((hzWsmoth[2] + hzWsmoth[4]) * (half)param->coef[2]) +
                     (double)half((hzWsmoth[3]              ) * (half)param->coef[3]));
      hp = half(in[3][x] - Wsmooth);
      denoised = half((float)((float)(in[2][x] * 1 / 8.0f + in[3][x-1] * 1 / 8.0f + in[4][x] * 1 / 8.0f + in[3][x+1] * 1 / 8.0f + in[3][x] * 4 / 8.0f)));
      // range part
      if(denoised < half(param->s[0]))
      {
         lut = denoised/param->s[0];
      }
      else 
      {
         if(denoised < half(param->s[1]))
         {
            lut = 1;
         }
         else 
         {
            if(denoised < half(param->s[2]))
            {
               lut = (param->s[2] - denoised) / (param->s[2] - param->s[1]);
            }
            else
            {
               lut = 0;
            }
         }
      }

      hp = hp * lut;
      hp = hp * param->strength;        
      hp = hp  > param->thresh ? hp - param->thresh : hp < -param->thresh ? hp + param->thresh : 0;
      wpoint = (float)in[3][x];
      // max/min calculation
      Wmax =  MAX(
                     MAX(
                           MAX(
                                 MAX(in[2][x-1], in[2][x]),
                                 MAX(in[2][x+1], in[3][x-1])
                              ),
                           MAX(
                                 MAX(in[3][x], in[3][x+1]),
                                 MAX(in[4][x-1], in[4][x])
                              )
                        ),
                     in[4][x+1]
                  );
      Wmin =  MIN(
                     MIN(
                           MIN(
                                 MIN(in[2][x-1], in[2][x]),
                                 MIN(in[2][x+1], in[3][x-1])
                              ),
                           MIN(
                                 MIN(in[3][x], in[3][x+1]),
                                 MIN(in[4][x-1], in[4][x])
                              )
                        ),
                     in[4][x+1]
                  );                      
      delta_max = (float)((Wmax - wpoint) * param->overshoot);
      delta_min = (float)((Wmin - wpoint) * undershoot);
      hp = hp > delta_max ? param->clipping * delta_max + (1-param->clipping) * hp  : hp < delta_min ?  param->clipping * delta_min + (1-param->clipping) * hp : hp ;
      wp = in[3][x];
      d = (float)hp; 
      wp += d;
      wp = wp < 0.0f ? 0 : wp > 1.0f ? 1.0f : wp;
      out[x] = (half)wp;
   }
} 
