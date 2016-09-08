//
//  BLECozmoRandom.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/19/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#include "BLECozmoRandom.h"

#import <Security/Security.h>

int BLECozmoGetRandomBytes(uint8_t* buffer, uint32_t numBytes)
{
  return SecRandomCopyBytes(kSecRandomDefault, numBytes, buffer);
}

