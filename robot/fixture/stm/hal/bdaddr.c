//#include <stdarg.h>
//#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "bdaddr.h"
#include "random.h"

//------------------------------------------------  
//    Validation
//------------------------------------------------

bool bdaddr_valid(bdaddr_t *bdaddr)
{
  //Core_v4.2: static address is a 48-bit randomly generated address and shall meet the following requirements:
  //• The two most significant bits of the address shall be equal to 1  
  //• All bits of the random part of the address shall not be equal to 1
  //• All bits of the random part of the address shall not be equal to 0
  
  uint32_t *a32 = (uint32_t*)bdaddr;
  
  if( !bdaddr || ((bdaddr->addr[5] & 0xC0) != 0xC0)  ||
      (a32[0] == 0xFFFFffff && (a32[1] & 0x3fff) == 0x3fff) ||
      (a32[0] == 0x00000000 && (a32[1] & 0x3fff) == 0x0000) )
  {
    return false;
  }
  return true;
}

void bdaddr_generate(bdaddr_t *out_bdaddr, uint32_t(*getRand)(void) )
{
  if( out_bdaddr && getRand )
  {
    do {
      for(int x=0; x < BDADDR_SIZE; x++)
        out_bdaddr->addr[x] = getRand();
      out_bdaddr->addr[BDADDR_SIZE-1] |= 0xC0; //2MSb required '11'
    }
    while( !bdaddr_valid(out_bdaddr) );
  }
}

//------------------------------------------------  
//    String methods
//------------------------------------------------

//XXX: I use this in multiple apps, all implemented in different places
extern char* snformat(char *s, size_t n, const char *format, ...);

char* bdaddr2str(bdaddr_t *bdaddr)
{
  static char str[BDADDR_STR_SIZE+1];
  if( bdaddr ) {
    snformat(str,sizeof(str),"%02x:%02x:%02x:%02x:%02x:%02x", bdaddr->addr[0], bdaddr->addr[1], bdaddr->addr[2], bdaddr->addr[3], bdaddr->addr[4], bdaddr->addr[5] );
    return str;
  }
  return 0;
}

bdaddr_t* str2bdaddr(char* str, bdaddr_t *out_bdaddr)
{
  if( out_bdaddr )
    memset(out_bdaddr, 0, sizeof(bdaddr_t) ); //always clear on entry
  
  if( out_bdaddr && str && strlen(str) >= BDADDR_STR_SIZE )
  {
    uint8_t* write = (uint8_t*)out_bdaddr;
    char* next = str;
    
    while( next < (str+BDADDR_STR_SIZE) && ((int)write-(int)out_bdaddr) < BDADDR_SIZE )
    {
      char *endptr;
      *write++ = strtol(next,&endptr,16); //endptr set to 'next character in str after the numerical value.' (e.g. ':' separator)
      next = endptr + 1;
    }
    
    //invalidate if we detect a format error
    if( //next != (str+BDADDR_STR_SIZE+1) || 
        ((int)write-(int)out_bdaddr) != BDADDR_SIZE )
    {
      memset(out_bdaddr, 0, sizeof(bdaddr_t) );
    }
  }
  return out_bdaddr;
}
