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

#import <UIKit/UIKit.h>

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
#import "CozmoVisionMarkerBBox.h"
#import "VerticalSliderView.h"

#endif

// TODO: Hook these up to settings?
const int GAME_LENGTH_SEC = 30;
const int START_NUM_HEALTH = 10;
const float SHOT_RELOAD_TIME = 0.5f;

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
@property (weak, nonatomic) IBOutlet UIButton *gameTimerButton;

@property (assign, nonatomic) Float32 markerIntensity;
@property (assign, nonatomic) int markerType;
@property (assign, nonatomic) Float32 markerDistanceFromCenterSq;
@property (assign, nonatomic) BOOL markerFound;
@property (assign, nonatomic) CGRect marker;

// Shooting game:
@property (assign, nonatomic) BOOL gameRunning;
@property (assign, nonatomic) int points;
@property (assign, nonatomic) int gameTimeRemaining;
@property (assign, nonatomic) BOOL isReloading;

// Audio targeting:
@property (strong, nonatomic) NSURL*          targetingSoundURL;
@property (strong, nonatomic) NSURL*          lockSoundURL;
@property (strong, nonatomic) AVAudioPlayer*  targetingSoundPlayer;
@property (assign, nonatomic) NSTimeInterval  targetingSoundPeriod_sec;

@property (strong, nonatomic) AVAudioPlayer*  lockSoundPlayer;
@property (assign, nonatomic) BOOL            targetLocked;
@property (assign, nonatomic) BOOL            runTimer;

@property (assign, nonatomic) Float32         targetingSlopFactor;

@property (weak, nonatomic) CozmoOperator*      cozmoOperator;
@property (weak, nonatomic) CozmoEngineWrapper* cozmoEngineWrapper;

@property (weak, nonatomic) IBOutlet UILabel *useVisualTargetingLabel;
@property (weak, nonatomic) IBOutlet UISwitch *useVisualTargetingSwitch;
@property (weak, nonatomic) IBOutlet UILabel *useAudioTargetingLabel;
@property (weak, nonatomic) IBOutlet UISwitch *useAudioTargetingSwitch;
@property (weak, nonatomic) IBOutlet VerticalSliderView *targetingSlopSlider;

- (void)playTargetingSound;
- (void)resetGame;
- (void)processVisionMarker:(CozmoVisionMarkerBBox*)marker;

@end

@implementation ShootingCozmoViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  
  self.view.backgroundColor = [UIColor blackColor];
  
  self.cozmoEngineWrapper = [CozmoEngineWrapper defaultEngine];
  self.cozmoOperator      = [CozmoOperator defaultOperator];
  
  [self.cozmoOperator setHandleDeviceDetectedVisionMarker:^(CozmoVisionMarkerBBox* marker){
    [self processVisionMarker:marker];
  }];

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

  // Create players for targeting and lock sounds
  NSString* platformSoundRootDir = [PlatformPathManager_iOS getPathWithScope:PlatformPathManager_iOS_Scope_Sound];
  self.targetingSoundURL = [NSURL URLWithString:[platformSoundRootDir stringByAppendingString:@"laser/blip.wav"]];
  NSError *error = nil;
  self.targetingSoundPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:self.targetingSoundURL error:&error];
  if (!error) {
    self.targetingSoundPlayer.volume = 0.25;
    self.targetingSoundPlayer.delegate = self;
    self.targetingSoundPlayer.numberOfLoops = 0;
  }
  
  self.lockSoundURL = [NSURL URLWithString:[platformSoundRootDir stringByAppendingString:@"laser/TargetLock.mp3"]];
  error = nil;
  self.lockSoundPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:self.lockSoundURL error:&error];
  if (!error) {
    self.lockSoundPlayer.delegate = nil;
    self.lockSoundPlayer.volume = 0.75;
    self.lockSoundPlayer.numberOfLoops = -1;
  }
  
  self.targetingSoundPeriod_sec = 0;
  self.targetLocked = NO;
  self.runTimer = YES;
  [self playTargetingSound];
  
  // Start up the vision thread for processing device camera images
  // TODO: Get calibration of device camera somehow
  Anki::Vision::CameraCalibration deviceCamCalib;
  
  self.targetingSlopSlider.title = @"Slop";
  [self.targetingSlopSlider setValueRange:1.0:3.0];
  self.targetingSlopSlider.valueChangedThreshold = 0.025;
  [self.targetingSlopSlider setActionBlock:^(float value) {
    self.targetingSlopFactor = value;
  }];
  
  self.useAudioTargetingLabel.textColor = self.useAudioTargetingLabel.tintColor;
  self.useVisualTargetingLabel.textColor = self.useVisualTargetingLabel.tintColor;
  
  // Reload last setup
  self.useVisualTargeting = [NSUserDefaults lastUseVisualTargeting];
  self.useAudioTargeting  = [NSUserDefaults lastUseAudioTargeting];
  self.targetingSlopFactor = [NSUserDefaults lastTargetingSlopFactor];
  self.useAudioTargetingSwitch.on = self.useAudioTargeting;
  self.useVisualTargetingSwitch.on = self.useVisualTargeting;
  self.cameraView.hidden = !self.useVisualTargeting;
  [self.targetingSlopSlider setValue:self.targetingSlopFactor];

  // Set up taps for firing
  UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
  [self.view addGestureRecognizer:tapGesture];
  
  // Set up swipe right to exit (in addition to exit button)
  UISwipeGestureRecognizer *swipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self  action:@selector(handleExitButtonPress:)];
  swipeGesture.direction = UISwipeGestureRecognizerDirectionRight;
  [self.view addGestureRecognizer:swipeGesture];
  
  [self resetGame];
  [_videoCamera start];
}

- (void) viewDidDisappear:(BOOL)animated
{
  [NSUserDefaults setLastUseVisualTargeting:self.useAudioTargeting];
  [NSUserDefaults setLastUseAudioTargeting:self.useVisualTargeting];
  [NSUserDefaults setLastTargetingSlopFactor:self.targetingSlopFactor];
  
  self.runTimer = NO;
  self.targetingSoundPeriod_sec = 0;
  [self.targetingSoundPlayer stop];
  [self.lockSoundPlayer stop];
  
  [_videoCamera stop];
}

- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscapeRight;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
  return UIInterfaceOrientationLandscapeRight;
}

- (void)updateCrosshairsWithMarkerDetected:(BOOL)markerDetected distance:(Float32)distance
{
  #define maxDistance 130.0
  #define lockIntensity 0.9
  
  Float32 intensity  = 0.f;
  Float32 period_sec = 0.f;
  
  if (markerDetected) {
    intensity = (distance > maxDistance) ? 0.0 : 1 - (distance / maxDistance);
    
    period_sec = distance/maxDistance + 0.01;
    self.targetingSoundPeriod_sec = 0.5f * (period_sec + self.targetingSoundPeriod_sec);
    
  } else {
    // Want this to be exactly zero when marker wasn't detected to disable targeting sound
    self.targetingSoundPeriod_sec = 0.f;
  }

  // Average prev & new intensity/period
  self.markerIntensity = 0.5f * (intensity + self.markerIntensity);
  
  if(self.useVisualTargeting) {
    dispatch_async(dispatch_get_main_queue(), ^{
      self.crosshairTopImageView.alpha = self.markerIntensity;
    });
  }
}

- (void)animateCrossHairsFire
{
  // Only show firing animation while using visual targeting
  if(self.useVisualTargeting) {
    CABasicAnimation* animation = [CABasicAnimation animationWithKeyPath:@"transform"];
    animation.fromValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.0, 1.0, 1.0)];
    animation.toValue = [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.1, 1.1, 1.0)];
    animation.duration = 0.1;
    [animation setFillMode:kCAFillModeRemoved];
    
    [self.crosshairsImageView.layer addAnimation:animation forKey:@"transform"];
  }
}

- (void)animateTargetHit
{
  // Only show hit explosion when using visual targeting
  if(self.useVisualTargeting) {
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
}

#pragma mark - Protocol CvVideoCameraDelegate

- (void) processVisionMarker:(CozmoVisionMarkerBBox *) currentMarker
{
  self.markerFound = YES;
  
  // Add targeting slop, if any
  if(self.targetingSlopFactor > 1.f) {
    const Float32 adjustment = (self.targetingSlopFactor - 1.f)*0.5f;
    Float32 dx = CGRectGetWidth(currentMarker.boundingBox)*adjustment;
    Float32 dy = CGRectGetHeight(currentMarker.boundingBox)*adjustment;
    currentMarker.boundingBox = CGRectInset(currentMarker.boundingBox, -dx, -dy);
  }
  
  // Distance of the targeting point (center of image) to the rectangle's border
  const Float32 xCen = CGRectGetMidX(currentMarker.boundingBox);
  const Float32 yCen = CGRectGetMidY(currentMarker.boundingBox);
  const Float32 halfWidth = 0.5f*CGRectGetWidth(currentMarker.boundingBox);
  const Float32 halfHeight = 0.5f*CGRectGetHeight(currentMarker.boundingBox);
  const Float32 xDistance = fmaxf(0.f, fabsf(160.f-xCen) - halfWidth);
  const Float32 yDistance = fmaxf(0.f, fabsf(120.f-yCen) - halfHeight);
  const Float32 currentDistanceSq = xDistance*xDistance + yDistance*yDistance;
  
  // Keep the marker in view that's closest to center
  if(currentDistanceSq < self.markerDistanceFromCenterSq) {
    self.marker = currentMarker.boundingBox;
    self.markerType = currentMarker.markerType; //msg.markerType;
    self.markerDistanceFromCenterSq = currentDistanceSq;
  }
  
}

#ifdef __cplusplus
- (void)processImage:(cv::Mat&)imageBGR
{
  using namespace Anki;
  using namespace cv;

  // Make sure we at least call VisionThread->SetNextImage the first call!
  //static BOOL calledOnce = NO;

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
/*
  if(!calledOnce || [self.cozmoEngineWrapper wasLastDeviceImageProcessed])
  {
    calledOnce = YES;
 
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
      if (self.useAudioTargeting ) {
        // For audio targeting, it is sufficient for the marker to be visible
        // at all in the image.
        markerInCrosshairs = YES;
      } else {
        // See if the crosshairs at center of image is within the marker's
        // bounding box
        markerInCrosshairs = CGRectContainsPoint(marker, CGPointMake(160,120));
      }
      
      if(markerInCrosshairs) {
        self.markerType = markerTypeOut; //msg.markerType;
 
        Float32 xDistance = CGRectGetMidX(marker) - 160.0;
        Float32 yDistance = CGRectGetMidY(marker) - 120.0;
        markerDistanceFromCenter = sqrt(xDistance * xDistance + yDistance * yDistance);
        break;
      }
    }

    [self updateCrosshairsWithMarkerDetected:markerInCrosshairs distance:markerDistanceFromCenter];
*/
    Mat_<u8> imageGrayROI(240,320);
    if (self.useAudioTargeting) {
      // Detect the marker from anywhere in the field of view (using lower-res image for now)
      cv::resize(imageGray, imageGrayROI, cv::Size(320,240));
    } else {
      // Only detect the marker in a central ROI of the image
      
      // TODO: Use ROI feature of cv::Mat instead of copying the data out
      // (this will require support for non-continuous image data inside the VisionSystem)
      
      for(s32 i=0; i<240; ++i) {
        u8* imageGrayROI_i = imageGrayROI[i];
        u8* imageGray_i = imageGray[detectRectPt1.y + i];
        for(s32 j=0; j<320; ++j) {
          imageGrayROI_i[j] =imageGray_i[detectRectPt1.x + j];
        }
      }
    }

  // Detection rectangle is red and label says "No Marker" unless we find a
  // vision marker below
  self.markerDistanceFromCenterSq = 1000000.0;
  self.markerFound = NO;
  self.markerType = Anki::Vision::MARKER_UNKNOWN;
  
  // Use a bit hysteresis on the marker being found
  static int framesSinceMarkerFound = 0;
  static Float32 lastMarkerDistanceFromCenterSq = self.markerDistanceFromCenterSq;
  static BOOL wasTargetLocked = NO;
  const int numHysteresisFrames = 5;

  // Process the image. The vision marker, if any, closest to center will be
  // left in self.marker, thanks to signals/callbacks.
  Vision::Image ankiImage(imageGrayROI);
  [self.cozmoEngineWrapper processDeviceImage:ankiImage];
  
  // TODO: get rid of this polling
  NSArray* markers = [self.cozmoEngineWrapper getVisionMarkersDetectedByDevice];
  if(markers) {
    for(CozmoVisionMarkerBBox* marker in markers) {
      [self processVisionMarker:marker];
    }
  }
  
  
  // Once processing completes, check to see if we found a marker (this flag gets
  // set by the calls to processVisionMarker() above.)
  // TODO: set this flag by signals/messages from within the engine
  if(self.markerFound) {

    // If we found a marker, the crosshairs must be within it to be in "lock"
    self.targetLocked = CGRectContainsPoint(self.marker, CGPointMake(160,120));
    
    if(self.useAudioTargeting == NO) {
      // For audio targeting, it is sufficient for the marker to be visible
      // at all in the image, but otherwise, the marker must also be in lock.
      self.markerFound = self.targetLocked;
    }
    
    // Reset hysteresis stuff
    framesSinceMarkerFound = 0;
    lastMarkerDistanceFromCenterSq = self.markerDistanceFromCenterSq;
    wasTargetLocked = self.targetLocked;
    
  } else {
    
    ++framesSinceMarkerFound;
    
    if(framesSinceMarkerFound > numHysteresisFrames) {
      // It's been long enough that we haven't seen a marker to consider it lost
      self.markerFound = NO;
      lastMarkerDistanceFromCenterSq = 100000.f;
      wasTargetLocked = NO;
      
      // If we lost the marker, make sure we turn off "lock"
      self.targetLocked = NO;
    } else {
      // Still within the hysteresis, so pretend we're still seeing the marker
      // at the last known distance
      self.markerFound = YES;
      self.targetLocked = wasTargetLocked;
      self.markerDistanceFromCenterSq = lastMarkerDistanceFromCenterSq;
    }
  }
  
  if(self.lockSoundPlayer.isPlaying && self.targetLocked == NO) {
    [self.lockSoundPlayer pause];
  }
  
  [self updateCrosshairsWithMarkerDetected:self.markerFound distance:sqrt(self.markerDistanceFromCenterSq)];
  
  
//}

  // Invert image - why? just cause it looks neat.
//  bitwise_not(imageGray, imageGray);

  //Convert BGRA for display
//  cvtColor(imageGray, imageBGR, COLOR_GRAY2BGRA);

  // Overlay the detection rectangle on the image, in the color determined above
//  rectangle(imageBGR, detectRectPt1, detectRectPt2, detectionRectColor, 4);

}
#endif


#pragma mark - Audio Targeting

- (void) playTargetingSound
{
  if(self.targetLocked == YES) {
    // If the targeting sound is currently playing, stop it
    if(self.targetingSoundPlayer.isPlaying) {
      [self.targetingSoundPlayer stop];
    }
    // If the lock sound isn't already playing, start it
    if(self.lockSoundPlayer.isPlaying == NO) {
      [self.lockSoundPlayer play];
    }
  } else if(self.targetingSoundPeriod_sec > 0) {
    // If the lock sound is playing, pause it
    if(self.lockSoundPlayer.isPlaying) {
      [self.lockSoundPlayer pause];
    }
    // If the targeting sound isn't already playing, play it
    if(self.targetingSoundPlayer.isPlaying == NO) {
      [self.targetingSoundPlayer play];
    }
  }
  
  // Keep the timers running as long as the shooter view is up and running,
  // just use the current period
  if(self.runTimer) {
    // NOTE: Due to innaccuracy of NSTimer (I think?), can't let sound period get
    //  arbitrarily low. So I'm using an empirical minimum value here.
    double nextTime = fmax(self.targetingSoundPeriod_sec, 0.07) + self.targetingSoundPlayer.duration;
    [NSTimer scheduledTimerWithTimeInterval:nextTime target:self selector:@selector(playTargetingSound) userInfo:nil repeats:NO];
  }
}

- (void) resetGame
{
  self.gameRunning = NO;
  [self.gameTimerButton setTitle:@"Start" forState:UIControlStateNormal];
  self.gameTimeRemaining = GAME_LENGTH_SEC + 1;
  self.points = START_NUM_HEALTH;
  self.pointsLabel.hidden = YES;
  self.pointsLabel.text = [NSString stringWithFormat:@"Health: %d", self.points];
  self.isReloading = NO;
}

- (void)reloadCompleted:(NSTimer*)timer
{
  self.isReloading = NO;
}

#pragma mark - Action Methods

- (void)handleTapGesture:(UITapGestureRecognizer*)gesture
{
  // Can't shoot while reloading
  if(self.isReloading == YES)
    return;
  
  /*
  if (self.markerIntensity > 0.5) {
    Float32 pointsMultiplier = 1.0;
    if(self.markerType == Anki::Vision::MARKER_SPIDER) {
      // Special animation for spider
      [self.cozmoOperator sendAnimationWithName:@"ANIM_GOT_SHOT2"];
      pointsMultiplier = 100.0;
    } else if(self.markerType != Anki::Vision::MARKER_UNKNOWN) {
      [self.cozmoOperator sendAnimationWithName:@"ANIM_GOT_SHOT"];
      pointsMultiplier = 25;
    }
    [self animateTargetHit];

    // Get points based on size (if you hit a larger marker, you get fewer points
    // because it's easier)
    Float32 markerSize = CGRectGetHeight(self.marker) * CGRectGetWidth(self.marker);
    self.points += pointsMultiplier*(1.0 - markerSize / (320*240));
    self.pointsLabel.text = [NSString stringWithFormat:@"Points: %d", self.points];
  }
   */
  
  // Trigger a reload so we can't fire again until it is done
  self.isReloading = YES;
  [NSTimer scheduledTimerWithTimeInterval:SHOT_RELOAD_TIME target:self selector:@selector(reloadCompleted:) userInfo:nil repeats:NO];
  
  [[SoundCoordinator defaultCoordinator] playSoundWithFilename:@"laser/LaserFire.wav"
                                                        volume:0.5
                                                     withDelay:0
                                                      numLoops:0];
  
  if(self.gameRunning && self.targetLocked) {
    
    [self.cozmoOperator sendAnimationWithName:@"ANIM_GOT_SHOT"];
    [self animateTargetHit];

    --self.points;
    self.pointsLabel.text = [NSString stringWithFormat:@"Health: %d", self.points];
    if(self.points == 0) {
      self.gameRunning = NO;
      
      UIAlertController * alert=   [UIAlertController
                                    alertControllerWithTitle:@"Game Over"
                                    message:@"Code monster destroyed!"
                                    preferredStyle:UIAlertControllerStyleAlert];
      
      UIAlertAction* ok = [UIAlertAction
                           actionWithTitle:@"OK"
                           style:UIAlertActionStyleDefault
                           handler:^(UIAlertAction * action)
                           {
                             [self resetGame];
                             [alert dismissViewControllerAnimated:YES completion:nil];
                           }];
      
      [alert addAction:ok];
      [self presentViewController:alert animated:YES completion:nil];
    }
  }
  
  if(self.useVisualTargeting) {
    [self.bulletOverlayView fireLaserButtlet];
    [self animateCrossHairsFire];
  }
}

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)handleUseVisualTargetingSwitch:(UISwitch *)sender {
  self.useVisualTargeting = sender.isOn;

  // Hide camera view if not using visual targeting
  self.cameraView.hidden = !self.useVisualTargeting;
  
  // Set to full alpha for crosshairs if not using visual targeting (since we
  // won't be animating it)
  if(self.useVisualTargeting == NO) {
    self.crosshairTopImageView.alpha = 1.0;
  }
}

- (IBAction)handleUseAudioTargetingSwitch:(UISwitch *)sender {
  self.useAudioTargeting = sender.isOn;
}

- (void) gameTimerCountdown {

  self.gameTimeRemaining -= 1;
  [self.gameTimerButton setTitle:[NSString stringWithFormat:@"%d",self.gameTimeRemaining] forState:UIControlStateNormal];
  
  if(self.gameTimeRemaining > 0) {
    if(self.gameRunning) {
      [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(gameTimerCountdown) userInfo:nil repeats:NO];
    }
  } else {
    self.gameRunning = NO;
    
    UIAlertController * alert=   [UIAlertController
                                  alertControllerWithTitle:@"Game Over"
                                  message:@"Code monster exhausted Cozmo!"
                                  preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction* ok = [UIAlertAction
                         actionWithTitle:@"OK"
                         style:UIAlertActionStyleDefault
                         handler:^(UIAlertAction * action)
                         {
                           [self resetGame];
                           [alert dismissViewControllerAnimated:YES completion:nil];
                         }];
    
    [alert addAction:ok];
    [self presentViewController:alert animated:YES completion:nil];
  }
}

- (IBAction)handleGameTimerButtonPress:(UIButton *)sender {
  self.gameRunning = YES;
  self.pointsLabel.hidden = NO;
  [self gameTimerCountdown];
}

@end
