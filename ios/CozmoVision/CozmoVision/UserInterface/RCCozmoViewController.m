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
#import "CozmoObsObjectBBox.h"

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
    
    NSArray *objectBBoxes = [self._basestation boundingBoxesObservedByRobotId:1];
    
    ///////
    // TODO: This is temporary drawing code
    // Drawing code
    for(CozmoObsObjectBBox *object in objectBBoxes)
    {
      CGContextRef context = UIGraphicsGetCurrentContext();
      CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 0.0);   //this is the transparent color
      CGContextSetRGBStrokeColor(context, 0.0, 0.0, 0.0, 0.5);
      CGContextFillRect(context, object.boundingBox);
      CGContextStrokeRect(context, object.boundingBox);    //this will draw the border
    }
    //////
  }
}

@end
