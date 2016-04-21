//
//  BLECozmoRandom.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/19/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#ifndef BLECozmoRandom_h
#define BLECozmoRandom_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BLECozmoGetRandomBytes(uint8_t* buffer, uint32_t numBytes);
  
#ifdef __cplusplus
}
#endif


#endif // BLECozmoRandom_h