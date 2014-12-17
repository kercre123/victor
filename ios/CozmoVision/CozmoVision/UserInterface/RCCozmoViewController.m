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
#import "CozmoObsObjectBBox.h"

#import "VerticalSliderView.h"
#import "VirtualDirectionPadView.h"

@interface RCCozmoViewController () <CozmoBasestationHeartbeatListener>
@property (weak, nonatomic) IBOutlet UIImageView *cozmoVisionImageView;
@property (weak, nonatomic) IBOutlet VerticalSliderView *headSlider;
@property (weak, nonatomic) IBOutlet VerticalSliderView *liftSlider;
@property (weak, nonatomic) IBOutlet VirtualDirectionPadView *dPadView;

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
  self.headSlider.title = @"Head";
  self.headSlider.valueChangedThreshold = 0.025;
  [self.headSlider setActionBlock:^(float value) {
    [self._operator sendHeadCommandWithAngleRatio:value];
  }];

  self.liftSlider.title = @"Lift";
  self.liftSlider.valueChangedThreshold = 0.025;
  [self.liftSlider setActionBlock:^(float value) {
    [self._operator sendLiftCommandWithHeightRatio:value];
  }];

  self.cozmoVisionImageView.contentMode = UIViewContentModeScaleAspectFit;

  [self.view insertSubview:self.dPadView aboveSubview:self.cozmoVisionImageView];

}

- (void)viewWillAppear:(BOOL)animated
{
  [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeRight animated:YES];

  [super viewWillAppear:animated];
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

- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscapeRight;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
  return UIInterfaceOrientationLandscapeRight;
}

- (BOOL)shouldAutorotate
{
  return YES;
}


#pragma mark - CozmoBasestationHeartbeatListener Methods

- (void)cozmoBasestationHearbeat:(CozmoBasestation *)basestation
{
  // Update Image Frame
  UIImage *updatedFrame = [self._basestation imageFrameWtihRobotId:1];
  if (updatedFrame) {
    
    NSArray *objectBBoxes = [self._basestation boundingBoxesObservedByRobotId:1];
    
    if(objectBBoxes && objectBBoxes.count > 0) {
      ///////
      // TODO: This is temporary drawing code
      // Drawing code
      
      // begin a graphics context of sufficient size
      UIGraphicsBeginImageContext(updatedFrame.size);
      
      // draw original image into the context
      [updatedFrame drawAtPoint:CGPointZero];
      
      // get the context for CoreGraphics
      CGContextRef ctx = UIGraphicsGetCurrentContext();
      

      CGContextSetLineWidth(ctx, 5.0);
      
      
      //UIFont *font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
      UIFont *font = [UIFont fontWithName:@"AvenirNextCondensed-Bold" size:50];
      
      NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
      paragraphStyle.alignment = NSTextAlignmentCenter;
      
      NSDictionary *attributes = @{ NSFontAttributeName:font,
                                    NSParagraphStyleAttributeName:paragraphStyle,
                                    NSForegroundColorAttributeName: [UIColor greenColor]};

      CGContextSetRGBStrokeColor(ctx, 0.0, 1.0, 0.0, 1.0);

      for(CozmoObsObjectBBox *object in objectBBoxes)
      {
        NSString* label = [NSString stringWithFormat:@"%d", object.objectID];

        [label drawInRect:CGRectIntegral(object.boundingBox) withAttributes:attributes];
        
        CGContextStrokeRect(ctx, object.boundingBox);
      }
      
      // make image out of bitmap context
      updatedFrame = UIGraphicsGetImageFromCurrentImageContext();
      
      // free the context
      UIGraphicsEndImageContext();
      
      // End of temporary drawing code
      //////
    } else {
      NSLog(@"No objects visible\n");
    } // if objectBBoxes
    
    self.cozmoVisionImageView.image = updatedFrame;
  } else {
    //NSLog(@"frame not updated\n");
  }

  // TODO: TEMP Solution to driving robot
//  [self._operator sendWheelCommandWithLeftSpeed:self.dPadView.leftWheelSpeed_mmps right:self.dPadView.rightWheelSpeed_mmps];
  [self._operator sendWheelCommandWithAngleInDegrees:self.dPadView.angleInDegrees magnitude:self.dPadView.magnitude];
}



#pragma mark - Handle Action Methods

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:^{

  }];
}


@end
