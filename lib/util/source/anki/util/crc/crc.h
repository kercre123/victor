//
//  crc.h
//  RushHour
//
//  Created by Brian Chapados on 5/30/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//


#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern uint16_t update_crc_ccitt( uint16_t crc, char c );
extern uint16_t calculate_crc_ccitt(uint16_t seed, const uint8_t* bytes, const uint32_t length);

#ifdef __cplusplus
}
#endif
  
#endif // CRC_H_
