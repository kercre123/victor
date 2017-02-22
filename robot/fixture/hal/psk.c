#include <stdint.h>
typedef uint32_t u32;
#include "random.h"

#define COZ15_PW_STRLEN 17
#define COZ15_PW_BUF_SZ (20) //20 bytes reserved

#define MIN_SEP 2  //dashes must be at least this many digits apart
#define MAX_CONSEC 8  //no more than this many digits between dashes

#define MIDDLE_LEN (COZ15_PW_STRLEN-2*MIN_SEP-1) //The number of positions where dashes may lie (10)

uint8_t getRandomDigit(void)
{
   static uint32_t pool;
   static uint8_t digit=0;
   uint32_t result;
   if (digit==0) {
      do {
         pool = GetRandom()*0x3FFFFFFF;  //rand bits in range 0..1.07e9
      } while (pool > (int)1e9);  //resample if above 1e9 to ensure even distribution.
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

//ensure no more than MAX_CONSEC chars between r1 and r2,
// and no more than MAX_CONSEC-MIN_SEP before r1, or after r2.
#define READABLE(r1,r2) ( ((r2)-(r1)<=MAX_CONSEC+1) && ((r1)<=MAX_CONSEC-MIN_SEP) && (MIDDLE_LEN-(r2)<=MAX_CONSEC-MIN_SEP))

//This selects 2 random values in the range [0..COZ15_PW_STRLEN) with at least MIN_SEP digits separating them
//Returns the selected values in v1 and v2;
void getDashLocs(int* v1, int* v2)
{
  uint8_t r1,r2;
  uint8_t separation = MIN_SEP;
  uint8_t range_max = MIDDLE_LEN; //subtract MIN_SEP chars from each end, dashes will never be there.
  int i;
  while (1) {
      uint32_t rand_bits = GetRandom();
     
      for (i=0;i<4;i++) {
         //extract 2 4-bit numbers from the entropy stream
         r1 = rand_bits&0xF;
         r2 = (rand_bits>>4)&0xF;
         rand_bits>>=8;  //discard used bits.
         
         // ensure r1<=r2
         if (r2<r1) { uint8_t t = r2;r2=r1;r1=t; }
         // select this pair iff constraints met
         if ((r1<=range_max) &&
             (r2<=range_max) &&
             READABLE(r1,r2) && 
             (r1 < r2-separation ))
         {
            *v1 = r1 + MIN_SEP;
            *v2 = r2 + MIN_SEP;
            return;
         }
      }
   }
}

int generateCozmoPassword(char* pwOut, int len)
{
   int dash1, dash2, i;
   if (len<COZ15_PW_BUF_SZ) {
      return -1;
   }
   getDashLocs(&dash1, &dash2);
   for (i=0;i<COZ15_PW_STRLEN;i++)
   {
      if (i==dash1 || i==dash2) {
         pwOut[i]='-';
      }
      else {
         pwOut[i]='0' + getRandomDigit();
      }
   }
   //pad the rest with zeros
   for (;i<COZ15_PW_BUF_SZ;i++) {
      pwOut[i]='\0';
   }
   return 0;
}

