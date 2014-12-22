//
//  SoundCoordinator.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/17/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SoundCoordinator : NSObject

+ (instancetype)defaultCoordinator;

- (void)playSoundWithFilename:(NSString*)filename;

- (void)playSoundWithFilename:(NSString*)filename volume:(float)volume;

@end
