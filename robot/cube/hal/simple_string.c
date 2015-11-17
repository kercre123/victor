#include "hal.h"

void simple_memset(s8* s1, s8 val, u8 n)
{
  u8 i;
  for(i = 0; i<n; i++)
  {
    s1[i] = val;
  }
  return;
}


void simple_memcpy(s8* s1, s8* s2, u8 n)
{
  u8 i;
  for(i = 0; i<n; i++)
  {
    s1[i] = s2[i];
  }  
  return;
}