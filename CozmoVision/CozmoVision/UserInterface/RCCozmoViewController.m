//
//  RCCozmoViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "RCCozmoViewController.h"
#import "CozmoBasestation.h"
#import "CozmoDirectionController.h"

@interface RCCozmoViewController () <CozmoBasestationHeartbeatListener>
@property (weak, nonatomic) IBOutlet UIImageView *cozmoVisionImageView;

@property (weak, nonatomic) CozmoBasestation *_basestation;

@end

@implementation RCCozmoViewController

- (void)viewDidLoad
{
  self._basestation = [CozmoBasestation defaultBasestation];


}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self._basestation addListener:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [self._basestation removeListener:self];
  [super viewWillDisappear:animated];
}



#pragma mark - CozmoBasestationHeartbeatListener Methods

- (void)cozmoBasestationHearbeat:(CozmoBasestation *)basestation
{
  // Update Image Frame
  UIImage *updatedFrame = [self._basestation imageFrameWtihRobotId:1];
  if (updatedFrame) {
    self.cozmoVisionImageView.image = updatedFrame;
  }
}

@end
