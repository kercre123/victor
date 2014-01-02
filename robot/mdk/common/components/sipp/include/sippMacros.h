#ifndef __SIPP_MACROS_H__
#define __SIPP_MACROS_H__

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static inline UInt8 clampU8(float in)
{
    in += 0.5; //round to neareset int
    if(in< 0)  return 0;
    if(in>255) return 255;
               return (UInt8)in;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static half clamp01(half x)
{
    if     (x < 0.0f) x = 0.0f;
    else if(x > 1.0f) x = 1.0f;
    return x;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static half clamp01ForP(half x)
{
   if  (x < 0.0f) {
      x = 0.0f;
   } else {
      if(x >= 1.0f) {
         x = 0.9995117f; // first value below 1 in fp16 
      }
   }
   return x;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static inline UInt8 clampZ255(half x)
{
    if(x <   0.0f) x =   0.0f;
    if(x > 255.0f) x = 255.0f;

    return(UInt8)x;
}

#endif // !__SIPP_MACROS_H__ 
