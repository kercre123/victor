#include <stdint.h>
typedef uint32_t u32;
#include "random.h"

#define COZ15_PW_STRLEN 17
#define COZ15_PW_BUF_SZ (20) //20 bytes reserved

#define MIN_SEP 2
#define MAX_STR 8

#define MIDDLE_LEN (COZ15_PW_STRLEN-2*MIN_SEP-1) //10

uint8_t getRandomDigit(void)
{
   static uint32_t pool;
   static uint8_t digit=0;
   uint32_t result;
   if (digit==0) {
      do {
         pool = GetRandom();
      } while (pool > (int)1e9);
   }
   result = pool%10;
   pool/=10;
   if (++digit>=9) {
      digit=0;
   }
   return result;
}

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define READABLE(r1,r2) ( ((r2)-(r1)<=MAX_STR+1) && ((r1)<=MAX_STR-MIN_SEP) && ((r2)>=MIDDLE_LEN+MIN_SEP-MAX_STR))



void getDashLocs(int* v1, int* v2)
{
  uint8_t r1,r2;
  int i;
  while (1) {
      uint32_t v = GetRandom();
     
      for (i=0;i<4;i++) {
         r1 = v&0xF;
         r2 = (v>>4&0xF);
         if (r2<r1) { uint8_t t = r2;r2=r1;r1=t; }
         if ((r1<=*v1) &&
             (r2<=*v1) &&
             READABLE(r1,r2) &&
             (r1 < r2-*v2 || r2 > r1+*v2))
         {
            *v1 = r1;
            *v2 = r2;
            return;
         }
         v>>=8;
      }
   }
}



int  generateCozmoPassword(char* pwOut, int len)
{
   int dash1  = MIDDLE_LEN, dash2 = MIN_SEP;
   int i=0;
   if (len<COZ15_PW_BUF_SZ) {
      return -1;
   }
   getDashLocs(&dash1, &dash2);
   dash1+=MIN_SEP;dash2+=MIN_SEP;
   for (i=0;i<COZ15_PW_STRLEN;i++)
   {
      if (i==dash1 || i==dash2) {
         pwOut[i]='-';
      }
      else {
         pwOut[i]='0'+getRandomDigit();
      }
   }
   //pad the rest with zeros
   for (;i<COZ15_PW_BUF_SZ;i++) {
      pwOut[i]='\0';
   }
   return 0;
}

