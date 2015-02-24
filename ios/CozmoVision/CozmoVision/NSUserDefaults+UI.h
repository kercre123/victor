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

+ (int)lastNumRobots;
+ (void)setLastNumRobots:(int)N;

+ (int)lastNumUiDevices;
+ (void)setLastNumUiDevices:(int)N;

+ (BOOL)lastUseVisualTargeting;
+ (void)setLastUseVisualTargeting:(BOOL)use;

+ (BOOL)lastUseAudioTargeting;
+ (void)setLastUseAudioTargeting:(BOOL)use;

+ (float)lastTargetingSlopFactor;
+ (void)setLastTargetingSlopFactor:(float)factor;

+ (BOOL)lastDoForceAdd;
+ (void)setLastDoForceAdd:(BOOL)forceAdd;

+ (NSString*)lastForceAddIP;
+ (void)setLastForceAddIP:(NSString*)forceAddIP;

@end
