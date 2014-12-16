//
//  RCCozmoViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "RCCozmoViewController.h"
#import "CozmoBasestation.h"
#import "CozmoOperator.h"

#import "VerticalSliderView.h"
#import "DirectionPadView.h"

@interface RCCozmoViewController () <CozmoBasestationHeartbeatListener>
@property (weak, nonatomic) IBOutlet UIImageView *cozmoVisionImageView;
@property (weak, nonatomic) IBOutlet VerticalSliderView *cozmoHeadSlider;
@property (weak, nonatomic) IBOutlet VerticalSliderView *cozmoLiftSlider;

@property (weak, nonatomic) CozmoBasestation *_basestation;
@property (strong, nonatomic) CozmoOperator *_operator;

@end

@implementation RCCozmoViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self._basestation = [CozmoBasestation defaultBasestation];
  self._operator = [CozmoOperator operatorWithBasestationIPAddress:self._basestation.basestationIP];


  // Setup UI
  self.navigationController.navigationBarHidden = YES;

  self.cozmoHeadSlider.title = @"Head";
  self.cozmoHeadSlider.valueChangedThreshold = 0.025;
  [self.cozmoHeadSlider setActionBlock:^(float value) {
    [self._operator sendHeadCommandWithAngleRatio:value];
  }];

  self.cozmoLiftSlider.title = @"Lift";
  self.cozmoLiftSlider.valueChangedThreshold = 0.025;
  [self.cozmoLiftSlider setActionBlock:^(float value) {
    [self._operator sendLiftCommandWithHeightRatio:value];
  }];

  self.cozmoVisionImageView.contentMode = UIViewContentModeScaleAspectFit;


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



#pragma mark - Handle Action Methods

- (IBAction)handleExitButtonPress:(id)sender
{
  [self.navigationController popViewControllerAnimated:YES];
  self.navigationController.navigationBarHidden = NO;
}


@end
