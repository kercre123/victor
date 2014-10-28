//
//  BLEManager.m
//  BLEManager
//
//  Created by Mark Pauley on 9/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import "BLEManager.h"
#import "BLECentralMultiplexer.h"

void BLEManagerInitialize() {
  [BLECentralMultiplexer sharedInstance];
}