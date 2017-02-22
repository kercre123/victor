/*  pwgen.c
 *  Test harness for  fixture psk generator 
 *
 * Author: Adam Shelly  - Feb 2017
 *
 * Usage:  pwgen [N]
 *    Generates 10^N sample passwords
 *    Default: N=0 (1 password)
 *
 * Compile with 
gcc pwgen.c -o pwgen
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <math.h>


   
uint32_t SecureRand32(void) {
   //TODO: replace with real secure generator.
   uint32_t rand;            
   int fp = open("/dev/urandom", O_RDONLY);            
   read(fp, &rand, sizeof(rand)) ;
   close(fp);
   return rand;
}
#define GetRandom SecureRand32

#include "../fixture/hal/psk.c"

int main(int argc, const char* argv[])
{
   long  power = argc>1 ?  atol(argv[1]) : 0;
   char pwBuf[COZ15_PW_BUF_SZ];
   long i;
   long reps = pow(10, power);
   for (i=0;i<reps;i++) {
      generateCozmoPassword(pwBuf, COZ15_PW_BUF_SZ);
      printf("%s\n", pwBuf);
   }
}
   
