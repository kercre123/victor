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
#import "ObservedObjectSelector.h"

@interface RCCozmoViewController () <CozmoBasestationHeartbeatListener>
@property (weak, nonatomic) IBOutlet UIImageView *cozmoVisionImageView;
@property (weak, nonatomic) IBOutlet VerticalSliderView *headSlider;
@property (weak, nonatomic) IBOutlet VerticalSliderView *liftSlider;
@property (weak, nonatomic) IBOutlet VirtualDirectionPadView *dPadView;
@property (weak, nonatomic) IBOutlet UISwitch *detectFacesSwitch;
- (IBAction)handelDetectFacesSwitch:(id)sender;

@property (strong, nonatomic) ObservedObjectSelector *obsObjSelector;

@property (weak, nonatomic) CozmoBasestation *_basestation;
@property (strong, nonatomic) CozmoOperator *_operator;

@end

@implementation RCCozmoViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self._basestation = [CozmoBasestation defaultBasestation];
  self._operator = self._basestation.cozmoOperator;


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

  self.obsObjSelector = [ObservedObjectSelector new];
  
  [self.view insertSubview:self.dPadView aboveSubview:self.cozmoVisionImageView];

  [self.dPadView setDoubleTapAction:^(UIGestureRecognizer *gesture) {
    CGPoint point = [gesture locationInView:self.cozmoVisionImageView];
    CGPoint normalizedPoint = CGPointMake(point.x / self.dPadView.bounds.size.width,
                                          point.y / self.dPadView.bounds.size.height);
    NSNumber *objectID = [self.obsObjSelector checkForSelectedObject:normalizedPoint];
    if(objectID) {
      // Send message
      NSLog(@"Selected Object %d\n", objectID.integerValue);
      [self._operator sendPickOrPlaceObject:objectID];
    }
  }];
  
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

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];
  
  //self.obsObjSelector.frame = self.cozmoVisionImageView.frame;
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
    
    self.obsObjSelector.observedObjects = objectBBoxes;
    
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
      
      // Do we need to release the context?
      // This suggests not:
      //      http://stackoverflow.com/questions/3725786/do-i-need-to-release-the-context-returned-from-uigraphicsgetcurrentcontext
      //CGContextRelease(ctx);

      // End of temporary drawing code
      //////
    }
    
    self.cozmoVisionImageView.image = updatedFrame;
  }

  [self._operator update];
  
  // TODO: TEMP Solution to driving robot
//  [self._operator sendWheelCommandWithLeftSpeed:self.dPadView.leftWheelSpeed_mmps right:self.dPadView.rightWheelSpeed_mmps];

  static BOOL wasActivelyControlling = NO;
  if(self.dPadView.isActivelyControlling)
  {
    [self._operator sendWheelCommandWithAngleInDegrees:self.dPadView.angleInDegrees magnitude:self.dPadView.magnitude];
    wasActivelyControlling = YES;
  } else if(wasActivelyControlling) {
    [self._operator sendStopAllMotorsCommand];
    wasActivelyControlling = NO;
  }
  
  //[self._operator sendPickUpObject:]
}



#pragma mark - Handle Action Methods

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:^{

  }];
}


- (IBAction)handelDetectFacesSwitch:(UISwitch*)sender {
  [self._operator sendEnableFaceTracking:sender.on];
  if(sender.on) {
    self.dPadView.userInteractionEnabled = NO;
  } else {
    self.dPadView.userInteractionEnabled = YES;
  }
}

@end
