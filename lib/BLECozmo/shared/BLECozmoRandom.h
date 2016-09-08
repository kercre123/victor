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

// Return count random bytes in *bytes, allocated by the caller.
// It is critical to check the return value for error
//  @result Return 0 on success or -1 if something went wrong, check errno
//    to find out the real error.
int BLECozmoGetRandomBytes(uint8_t* buffer, uint32_t numBytes);
  
#ifdef __cplusplus
}
#endif


#endif // BLECozmoRandom_h
