//
//  RCCozmoViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "RCCozmoViewController.h"
#import "CozmoEngineWrapper.h"
#import "CozmoOperator.h"
#import "CozmoObsObjectBBox.h"

#import "RobotAnimationSelectTableViewController.h"
#import "VerticalSliderView.h"
#import "VirtualDirectionPadView.h"
#import "ObservedObjectSelector.h"

@interface RCCozmoViewController () <CozmoEngineHeartbeatListener>
@property (weak, nonatomic) IBOutlet UIImageView *cozmoVisionImageView;
@property (weak, nonatomic) IBOutlet VerticalSliderView *headSlider;
@property (weak, nonatomic) IBOutlet VerticalSliderView *liftSlider;
@property (weak, nonatomic) IBOutlet VirtualDirectionPadView *dPadView;
@property (weak, nonatomic) IBOutlet UISwitch *detectFacesSwitch;

- (IBAction)handelDetectFacesSwitch:(id)sender;

@property (strong, nonatomic) ObservedObjectSelector *obsObjSelector;

@property (weak, nonatomic) CozmoEngineWrapper *cozmoEngineWrapper;
@property (strong, nonatomic) CozmoOperator *_operator;
@property (strong, nonatomic) NSMutableArray* objectBoundingBoxes;

@end

@implementation RCCozmoViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.cozmoEngineWrapper = [CozmoEngineWrapper defaultEngine];
  self._operator = [CozmoOperator defaultOperator];


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

  [self.dPadView setJoystickMovementAction:^(CGFloat angle, CGFloat magnitude) {
    [self._operator sendWheelCommandWithAngleInDegrees:angle magnitude:magnitude];
  }];

  [self.dPadView setDoubleTapAction:^(UIGestureRecognizer *gesture) {
    CGPoint point = [gesture locationInView:self.cozmoVisionImageView];
    CGPoint normalizedPoint = CGPointMake(point.x / self.dPadView.bounds.size.width,
                                          point.y / self.dPadView.bounds.size.height);
    NSNumber *objectID = [self.obsObjSelector checkForSelectedObject:normalizedPoint];
    if(objectID) {
      // Send message
      NSLog(@"Selected Object %d\n", objectID.intValue);
      [self._operator sendPickOrPlaceObject:objectID];
    }
  }];
  

  self.objectBoundingBoxes = [[NSMutableArray alloc] init];
  
  __weak RCCozmoViewController* weakSelf = self;
  [self._operator setHandleRobotObservedObject:^(CozmoObsObjectBBox* observation) {
    //[weakSelf drawObservedObjectBoundingBox:observation];
    [weakSelf.objectBoundingBoxes addObject:observation];
  }];
  
  
} // viewDidLoad()

- (void)viewWillAppear:(BOOL)animated
{
  [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeRight animated:YES];

  [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self.cozmoEngineWrapper addHeartbeatListener:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [self.cozmoEngineWrapper removeHeartbeatListener:self];

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

- (void)cozmoEngineWrapperHeartbeat:(CozmoEngineWrapper *)basestation
{
  // Update Image Frame
  UIImage *updatedFrame = [self.cozmoEngineWrapper imageFrameWithRobotId:1];
  
  if (updatedFrame) {
    
    self.obsObjSelector.observedObjects = self.objectBoundingBoxes;
    
    if(self.objectBoundingBoxes.count > 0) {
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

      for(CozmoObsObjectBBox *object in self.objectBoundingBoxes)
      {
        NSString* label = [NSString stringWithFormat:@"%ld", (long)object.objectID];

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
      
      [self.objectBoundingBoxes removeAllObjects];
    }
    
    self.cozmoVisionImageView.image = updatedFrame;
  }
  
  static BOOL wasActivelyControlling = NO;
  if(self.dPadView.isActivelyControlling)
  {
    wasActivelyControlling = YES;
  } else if(wasActivelyControlling) {
    [self._operator sendStopAllMotorsCommand];
    wasActivelyControlling = NO;
  }
}



#pragma mark - Handle Action Methods

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:^{

  }];
}

- (IBAction)handleAnimationListButtonPress:(id)sender
{
  RobotAnimationSelectTableViewController* vc = [RobotAnimationSelectTableViewController new];
  __weak typeof(self) weakSelf = self;
  __weak typeof(vc) weakVC = vc;
  [vc setDidSelectAnimationAction:^(NSString *name) {
    [weakVC dismissViewControllerAnimated:YES completion:^{
      [weakSelf._operator sendAnimationWithName:name];
    }];
//    [self.navigationController popToViewController:self animated:YES];

  }];
  [self presentViewController:vc animated:YES completion:nil];
//  [self.navigationController pushViewController:vc animated:YES];
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
