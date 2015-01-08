//
//  NSUserDefaults+UI.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSUserDefaults (UI)

+ (NSString*)lastHostAdvertisingIP;
+ (void)setLastHostAdvertisingIP:(NSString*)hostAdvertisingIP;

// Default value is "127.0.0.1"
+ (NSString*)lastVizIP;
+ (void)setLastVizIP:(NSString*)vizIP;

+ (BOOL)cameraIsHighResolution;
+ (void)setCameraIsHighResolution:(BOOL)isHighResolution;

+ (NSInteger)lastNumRobots;
+ (void)setLastNumRobots:(NSInteger)N;

+ (NSInteger)lastNumUiDevices;
+ (void)setLastNumUiDevices:(NSInteger)N;

@end
