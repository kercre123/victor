//
//  ShootingCozmoViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "ShootingCozmoViewController.h"
#import <CoreGraphics/CoreGraphics.h>

#import <AVFoundation/AVFoundation.h>

#import "BulletOverlayView.h"
#ifdef __cplusplus
// For OpenCV Video processing of phone's camera:
#import <opencv2/opencv.hpp>
#import <opencv2/highgui/cap_ios.h>

#import <anki/vision/basestation/image_impl.h>
#import <anki/vision/basestation/cameraCalibration.h>
#import <anki/vision/MarkerCodeDefinitions.h>
#import <anki/common/basestation/platformPathManager_iOS_interface.h>

#import "SoundCoordinator.h"
#import "CozmoEngineWrapper.h"
#import "CozmoOperator.h"
#import "CvVideoCameraReorient.h"
#import "NSUserDefaults+UI.h"

#endif


@interface ShootingCozmoViewController () <CvVideoCameraDelegateReorient, AVAudioPlayerDelegate>
{
  CvVideoCameraReorient*                        _videoCamera;
}
@property (weak, nonatomic) IBOutlet UIView* cameraView;
@property (weak, nonatomic) IBOutlet BulletOverlayView* bulletOverlayView;
@property (weak, nonatomic) IBOutlet UIImageView *crosshairsImageView;
@property (weak, nonatomic) IBOutlet UIImageView *crosshairTopImageView;
@property (weak, nonatomic) IBOutlet UIImageView *targetHitImageView;
@property (weak, nonatomic) IBOutlet UILabel     *pointsLabel;

@property (assign, nonatomic) Float32 markerIntensity;
@property (assign, nonatomic) int markerType;
@property (assign, nonatomic) Float32 markerSize;

@property (assign, nonatomic) int points;

// Audio targeting:
@property (strong, nonatomic) AVAudioPlayer*  targetingSoundPlayer;
@property (assign, nonatomic) NSTimeInterval  targetingSoundPeriod_sec;
@property (assign, nonatomic) BOOL            targetingSoundCurrentlyPlaying;

@property (weak, nonatomic) CozmoOperator* _operator;
@property (weak, nonatomic) CozmoEngineWrapper* cozmoEngineWrapper;

@property (weak, nonatomic) IBOutlet UISwitch *useVisualTargetingSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *useAudioTargetingSwitch;

- (void)playTargetingSound;
  
@end

@implementation ShootingCozmoViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  
  self.view.backgroundColor = [UIColor blackColor];
  
  self.cozmoEngineWrapper = [CozmoEngineWrapper defaultEngine];
  self._operator = [self.cozmoEngineWrapper cozmoOperator];


  self.crosshairTopImageView.image = [self.crosshairTopImageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  self.crosshairTopImageView.tintColor = [UIColor redColor];

  self.targetHitImageView.layer.opacity = 0.0;


  // Set up videoCamera for displaying device's on-board camera, but don't start it
  self.cameraView.contentMode = UIViewContentModeCenter;
  _videoCamera = [[CvVideoCameraReorient alloc] initWithParentView:self.cameraView];
  _videoCamera.delegate = self;
  _videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
  _videoCamera.defaultAVCaptureSessionPreset  = AVCaptureSessionPreset640x480;
  _videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationLandscapeRight;
  _videoCamera.defaultFPS = 30;
  _videoCamera.grayscaleMode = NO;

  // Create Player and play sound
  NSString* platformSoundRootDir = [PlatformPathManager_iOS getPathWithScope:PlatformPathManager_iOS_Scope_Sound];
  NSString* urlStr = [platformSoundRootDir stringByAppendingString:@"demo/WaitingForDice1.wav"];
  NSError *error = nil;
  self.targetingSoundPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:urlStr] error:&error];
  if (!error) {
    // Created, add to active players
    self.targetingSoundPlayer.volume = 0.5;
    self.targetingSoundPlayer.delegate = self;
    self.targetingSoundPlayer.numberOfLoops = 0;
  }
  
  _targetingSoundPeriod_sec = 0;
  _targetingSoundCurrentlyPlaying = NO;
  
  // Start up the vision thread for processing device camera images
  // TODO: Get calibration of device camera somehow
  Anki::Vision::CameraCalibration deviceCamCalib;
  
  // Reload last setup
  self.useVisualTargeting = [NSUserDefaults lastUseVisualTargeting];
  self.useAudioTargeting  = [NSUserDefaults lastUseAudioTargeting];
  self.useAudioTargetingSwitch.on = self.useAudioTargeting;
  self.useVisualTargetingSwitch.on = self.useVisualTargeting;
  self.cameraView.hidden = !self.useVisualTargeting;

  
  UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
  [self.view addGestureRecognizer:tapGesture];
  
  [_videoCamera start];
}

- (void) viewDidDisappear:(BOOL)animated
{
  [NSUserDefaults setLastUseVisualTargeting:self.useAudioTargeting];
  [NSUserDefaults setLastUseAudioTargeting:self.useVisualTargeting];
}

- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscapeRight;
}
//
- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
  return UIInterfaceOrientationLandscapeRight;
}

- (void)updateCrosshairsWithMarkerDetected:(BOOL)markerDetected distance:(Float32)distance
{
  #define maxDistance 130.0
  
  Float32 intensity;

  if (markerDetected) {
    intensity = (distance > maxDistance) ? 1.0 : 1 - (distance / maxDistance);
    
    self.targetingSoundPeriod_sec = distance/maxDistance + 0.01;
  }
  else {
    intensity = 0.0;
    self.targetingSoundPeriod_sec = 0;
  }

  // Average prev & new intensity
  self.markerIntensity = (intensity + self.markerIntensity) / 2.0;
  
  // Won't do anything if period==0, nor if not using audio targeting
  [self playTargetingSound];


  dispatch_async(dispatch_get_main_queue(), ^{
    self.crosshairTopImageView.alpha = self.markerIntensity;
  });
}

- (void)animateCrossHairsFire
{
  CABasicAnimation* animation = [CABasicAnimation animationWithKeyPath:@"transform"];
  animation.fromValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.0, 1.0, 1.0)];
  animation.toValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.1, 1.1, 1.0)];
  animation.duration = 0.1;
  [animation setFillMode:kCAFillModeRemoved];

  [self.crosshairsImageView.layer addAnimation:animation forKey:@"transform"];
}

- (void)animateTargetHit
{
  // Delay explostion
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.22 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
  // Transform
  CABasicAnimation* animationTransform = [CABasicAnimation animationWithKeyPath:@"transform"];
  animationTransform.fromValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(0.20, 0.20, 1.0)];
  animationTransform.toValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.0, 1.0, 1.0)];
  animationTransform.duration = 0.2;
  [animationTransform setFillMode:kCAFillModeRemoved];

  CABasicAnimation* animationOpacity = [CABasicAnimation animationWithKeyPath:@"opacity"];
  animationOpacity.fromValue = @1.0;
  animationOpacity.toValue = @0.0;
  animationOpacity.duration = 0.4;
  [animationOpacity setFillMode:kCAFillModeRemoved];

  [self.targetHitImageView.layer addAnimation:animationTransform forKey:@"transfrom"];
  [self.targetHitImageView.layer addAnimation:animationOpacity forKey:@"opacity"];
  });
}

#pragma mark - Protocol CvVideoCameraDelegate

#ifdef __cplusplus
- (void)processImage:(cv::Mat&)imageBGR
{
  using namespace Anki;
  using namespace cv;

  // Make sure we at least call VisionThread->SetNextImage the first call!
  static BOOL calledOnce = NO;

  // Create an OpenCV grayscale image from the incoming data
  Mat_<u8> imageGray;
  cvtColor(imageBGR, imageGray, COLOR_BGR2GRAY);

  // Make this static so that it stays the same color between calls unless
  // updated via a detected marker in the IF below
  static Scalar detectionRectColor(0,0,255,255);

  // Detection rectangle is a QVGA window centered in the VGA frame:
  // TODO: Make smaller or square?
  Point2i   detectRectPt1(160,120);
  Point2i   detectRectPt2(detectRectPt1.x + 320, detectRectPt1.y + 240);

  if(!calledOnce || [self.cozmoEngineWrapper wasLastDeviceImageProcessed])
  {
    calledOnce = YES;
    cv::Rect  detectionRect(detectRectPt1, detectRectPt2);

    // Detection rectangle is red and label says "No Marker" unless we find a
    // vision marker below
    float markerDistanceFromCenter = 100000.0;
    BOOL markerInCrosshairs = NO;
    
    self.markerType = Anki::Vision::MARKER_UNKNOWN;
    
    CGRect marker;
    int markerTypeOut;
    
    while(YES == [self.cozmoEngineWrapper checkDeviceVisionMailbox:&marker
                                                      :&markerTypeOut])
    {
      // See if the crosshairs at center of image is within the marker's
      // bounding box
      markerInCrosshairs =  CGRectContainsPoint(marker, CGPointMake(160,120));
      if(markerInCrosshairs) {
        self.markerType = markerTypeOut; //msg.markerType;
        self.markerSize = CGRectGetHeight(marker) * CGRectGetWidth(marker);
        
        Float32 xDistance = CGRectGetMidX(marker) - 160.0;
        Float32 yDistance = CGRectGetMidY(marker) - 120.0;
        markerDistanceFromCenter = sqrt(xDistance * xDistance + yDistance * yDistance);
        break;
      }
    }

    [self updateCrosshairsWithMarkerDetected:markerInCrosshairs distance:markerDistanceFromCenter];

    // TODO: Use ROI feature of cv::Mat instead of copying the data out
    // (this will require support for non-continuous image data inside the VisionSystem)
    Mat_<u8> imageGrayROI(240,320);
    for(s32 i=0; i<240; ++i) {
      u8* imageGrayROI_i = imageGrayROI[i];
      u8* imageGray_i = imageGray[detectRectPt1.y + i];
      for(s32 j=0; j<320; ++j) {
        imageGrayROI_i[j] =imageGray_i[detectRectPt1.x + j];
      }
    }

    // Last image was processed (or this is the first call), so the engine
    // is ready for a new image:
    Vision::Image ankiImage(imageGrayROI);
    [self.cozmoEngineWrapper processDeviceImage:ankiImage];

  }

  // Invert image - why? just cause it looks neat.
//  bitwise_not(imageGray, imageGray);

  //Convert BGRA for display
//  cvtColor(imageGray, imageBGR, COLOR_GRAY2BGRA);

  // Overlay the detection rectangle on the image, in the color determined above
//  rectangle(imageBGR, detectRectPt1, detectRectPt2, detectionRectColor, 4);

}
#endif


#pragma mark - Audio Targeting

- (void)playTargetingSound {
  
  if(self.useAudioTargeting &&
     self.targetingSoundPeriod_sec > 0 &&
     self.targetingSoundCurrentlyPlaying == NO)
  {
    NSTimeInterval nextPlayTime = self.targetingSoundPlayer.deviceCurrentTime+self.targetingSoundPeriod_sec;
    BOOL soundPlayed = [self.targetingSoundPlayer playAtTime:nextPlayTime];
    if(soundPlayed == YES) {
      NSLog(@"Think i played targeting sound\n");
      self.targetingSoundCurrentlyPlaying = YES;
    } else {
      NSLog(@"Failed to play targeting sound.\n");
    }
  }
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
  // Keep playing with current frequency
  self.targetingSoundCurrentlyPlaying = NO;
  if(self.targetingSoundPeriod_sec > 0) {
    [self playTargetingSound];
  } else {
    [self.targetingSoundPlayer stop];
  }
}


#pragma mark - Action Methods


- (void)handleTapGesture:(UITapGestureRecognizer*)gesture
{
  if (self.markerIntensity > 0.5) {
    Float32 pointsMultiplier = 1.0;
    if(self.markerType == Anki::Vision::MARKER_SPIDER) {
      // Special animation for spider
      [self._operator sendAnimationWithName:@"ANIM_GOT_SHOT2"];
      pointsMultiplier = 100.0;
    } else if(self.markerType != Anki::Vision::MARKER_UNKNOWN) {
      [self._operator sendAnimationWithName:@"ANIM_GOT_SHOT"];
      pointsMultiplier = 25;
    }
    [self animateTargetHit];

    // Get points based on size (if you hit a larger marker, you get fewer points
    // because it's easier)
    self.points += pointsMultiplier*(1.0 - self.markerSize / (320*240));
    self.pointsLabel.text = [NSString stringWithFormat:@"Points: %d", self.points];
  }
  [[SoundCoordinator defaultCoordinator] playSoundWithFilename:@"laser/LaserFire.wav"
                                                        volume:0.5
                                                     withDelay:0
                                                      numLoops:0];
  [self.bulletOverlayView fireLaserButtlet];
  [self animateCrossHairsFire];
}

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)handleUseVisualTargetingSwitch:(UISwitch *)sender {
  self.useVisualTargeting = sender.isOn;

  // Hide camera view if not using visual targeting
  self.cameraView.hidden = !self.useVisualTargeting;
}

- (IBAction)handleUseAudioTargetingSwitch:(UISwitch *)sender {
  self.useAudioTargeting = sender.isOn;
}

@end
