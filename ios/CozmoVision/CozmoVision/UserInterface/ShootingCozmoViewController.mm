//
//  ShootingCozmoViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "ShootingCozmoViewController.h"
#import <CoreGraphics/CoreGraphics.h>

#import "BulletOverlayView.h"
#ifdef __cplusplus
// For OpenCV Video processing of phone's camera:
#import <opencv2/opencv.hpp>
#import <opencv2/highgui/cap_ios.h>

#import <anki/cozmo/basestation/visionProcessingThread.h>
#import <anki/vision/basestation/image_impl.h>
#import <anki/vision/MarkerCodeDefinitions.h>

#import "SoundCoordinator.h"
#import "CozmoBasestation.h"
#import "CozmoOperator.h"

#endif


@interface ShootingCozmoViewController () <CvVideoCameraDelegate>
{
  CvVideoCamera*                        _videoCamera;
  Anki::Cozmo::VisionProcessingThread*  _visionThread;
}
@property (weak, nonatomic) IBOutlet UIView* cameraView;
@property (weak, nonatomic) IBOutlet BulletOverlayView* bulletOverlayView;
@property (weak, nonatomic) IBOutlet UIImageView *crosshairsImageView;
@property (weak, nonatomic) IBOutlet UIImageView *crosshairTopImageView;
@property (weak, nonatomic) IBOutlet UIImageView *targetHitImageView;

@property (assign, nonatomic) Float32 markerIntensity;
@property (assign, nonatomic) float markerType;

@property (weak, nonatomic) CozmoOperator* _operator;
@end

@implementation ShootingCozmoViewController


- (void)viewDidLoad
{
  [super viewDidLoad];

  self.view.backgroundColor = [UIColor blackColor];
  self._operator = [[CozmoBasestation defaultBasestation] cozmoOperator];

  self.crosshairTopImageView.image = [self.crosshairTopImageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  self.crosshairTopImageView.tintColor = [UIColor redColor];

  self.targetHitImageView.layer.opacity = 0.0;


  // Set up videoCamera for displaying device's on-board camera, but don't start it
  self.cameraView.contentMode = UIViewContentModeCenter;
  _videoCamera = [[CvVideoCamera alloc] initWithParentView:_cameraView];
  _videoCamera.delegate = self;
  _videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
  _videoCamera.defaultAVCaptureSessionPreset  = AVCaptureSessionPreset640x480;
//  _videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationPortrait;
  _videoCamera.defaultFPS = 30;
  _videoCamera.grayscaleMode = NO;


  // Start up the vision thread for processing device camera images
  Anki::Vision::CameraCalibration deviceCamCalib;
  _visionThread = new Anki::Cozmo::VisionProcessingThread();
  _visionThread->Start(deviceCamCalib); // TODO: Only start when device camera selected?

  [_videoCamera start];

  UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
  [self.view addGestureRecognizer:tapGesture];
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
  }
  else {
    intensity = 0.0;
  }

  // Average prev & new intensity
  self.markerIntensity = (intensity + self.markerIntensity) / 2.0;

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

  if(!calledOnce || _visionThread->WasLastImageProcessed())
  {
    calledOnce = YES;
    cv::Rect  detectionRect(detectRectPt1, detectRectPt2);

    // Detection rectangle is red and label says "No Marker" unless we find a
    // vision marker below
    Cozmo::MessageVisionMarker msg;
    float closestDistance = 100000.0;
    BOOL markerDetected = NO;
    while(true == _visionThread->CheckMailbox(msg)) {

      Float32 xCenter = (msg.x_imgLowerLeft + msg.x_imgLowerRight + msg.x_imgUpperLeft + msg.x_imgUpperRight) * 0.25;
      Float32 yCenter = (msg.y_imgLowerLeft + msg.y_imgLowerRight + msg.y_imgUpperLeft + msg.y_imgUpperRight) * 0.25;
      Float32 xDistance = xCenter - 160.0;
      Float32 yDistance = yCenter - 120.0;
      Float32 currentDistance = sqrt(xDistance * xDistance + yDistance * yDistance);
      if (currentDistance < closestDistance) {
        closestDistance = currentDistance;
        markerDetected = YES;
        self.markerType = msg.markerType;
      }
    }

    [self updateCrosshairsWithMarkerDetected:markerDetected distance:closestDistance];

    // Process image within the detection rectangle with vision processing thread:
    static const Cozmo::MessageRobotState bogusState; // req'd by API, but not really necessary for marker detection

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

    // Last image was processed (or this is the first call), so the vision
    // thread is ready for a new image:
    Vision::Image ankiImage(imageGrayROI);
    _visionThread->SetNextImage(imageGrayROI, bogusState);

  }

  // Invert image - why? just cause it looks neat.
//  bitwise_not(imageGray, imageGray);

  //Convert BGRA for display
//  cvtColor(imageGray, imageBGR, COLOR_GRAY2BGRA);

  // Overlay the detection rectangle on the image, in the color determined above
//  rectangle(imageBGR, detectRectPt1, detectRectPt2, detectionRectColor, 4);

}
#endif

#pragma mark - Action Methods


- (void)handleTapGesture:(UITapGestureRecognizer*)gesture
{
  if (self.markerIntensity > 0.5) {
    [self._operator sendAnimationWithName:@"ANIM_GOT_SHOT"];
    [self animateTargetHit];
  }
  [[SoundCoordinator defaultCoordinator] playSoundWithFilename:@"laser/LaserFire.wav" volume:0.5];
  [self.bulletOverlayView fireLaserButtlet];
  [self animateCrossHairsFire];
}

- (IBAction)handleExitButtonPress:(id)sender
{
  [self dismissViewControllerAnimated:YES completion:nil];
}


@end
