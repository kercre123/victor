//
//  crc.c
//  RushHour
//
//  Created by Brian Chapados on 5/30/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <stdio.h>
#include "crc.h"

#define P_CCITT 0x1021

static uint8_t crc_tab_ccitt_init = 0;
static uint16_t crc_tab_ccitt[256];
static void init_crc_ccitt_tab( void );

uint16_t update_crc_ccitt( uint16_t crc, char c ) {
  
  unsigned short tmp, short_c;
  
  short_c  = 0x00ff & (uint16_t) c;
  
  if ( ! crc_tab_ccitt_init ) init_crc_ccitt_tab();
  
  tmp = (crc >> 8) ^ short_c;
  crc = (crc << 8) ^ crc_tab_ccitt[tmp];
  
  return crc;
  
}  /* update_crc_ccitt */

static void init_crc_ccitt_tab( void ) {
  
  int i, j;
  unsigned short crc, c;
  
  for (i=0; i<256; i++) {
    
    crc = 0;
    c   = ((unsigned short) i) << 8;
    
    for (j=0; j<8; j++) {
      
      if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ P_CCITT;
      else                      crc =   crc << 1;
      
      c = c << 1;
    }
    
    crc_tab_ccitt[i] = crc;
  }
  
  crc_tab_ccitt_init = 1;
  
}  /* init_crcccitt_tab */

uint16_t calculate_crc_ccitt(uint16_t seed, const uint8_t* bytes, const uint32_t length)
{
  uint16_t crc = seed;
  for (uint32_t i = 0; i < length; ++i) {
    crc = update_crc_ccitt(crc, bytes[i]);
  }
  return crc;
}
