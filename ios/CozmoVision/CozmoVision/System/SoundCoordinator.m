//
//  SoundCoordinator.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/17/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "SoundCoordinator.h"
#import <anki/common/basestation/platformPathManager_iOS_interface.h>
#import <AVFoundation/AVFoundation.h>

@interface SoundCoordinator () <AVAudioPlayerDelegate>
@property (copy, nonatomic) NSString* _platformSoundRootDir;
@property (strong, nonatomic) NSMutableArray* _activePlayers;

@end

@implementation SoundCoordinator

+ (instancetype)defaultCoordinator
{
  static SoundCoordinator *instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [SoundCoordinator new];
  });

  return instance;
}

- (instancetype)init
{
  if (!(self = [super init]))
    return self;

  self._platformSoundRootDir = [PlatformPathManager_iOS getPathWithScope:PlatformPathManager_iOS_Scope_Sound];

  self._activePlayers = [NSMutableArray arrayWithCapacity:10];

  return self;
}

- (void)playSoundWithFilename:(NSString*)filename
{
  [self playSoundWithFilename:filename volume:1.0];
}

- (void)playSoundWithFilename:(NSString*)filename volume:(float)volume
{
  // Create Player and play sound
  NSString* urlStr = [self._platformSoundRootDir stringByAppendingString:filename];
  NSError *error = nil;
  AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:urlStr] error:&error];
  if (!error) {
    // Created, add to active players
    player.volume = volume;
    player.delegate = self;
    [self._activePlayers addObject:player];
    [player play];
  }
}

#pragma mark - AVAudioPlayer Delegate Methods

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
  // Remove player from active players list
  [self._activePlayers removeObject:player];
}


@end
