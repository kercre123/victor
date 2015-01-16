//
//  ViewController.m
//  CozmoVision
//
//  Created by Andrew Stein on 11/25/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "ViewController.h"

#import <CoreGraphics/CoreGraphics.h>

#import "CozmoEngineWrapper.h"
#import "CozmoDirectionController.h"

#ifdef __cplusplus
// For OpenCV Video processing of phone's camera:
#import <opencv2/opencv.hpp>
#import <opencv2/highgui/cap_ios.h>

// For communicating UI Messages
#define COZMO_BASESTATION // to make uiMessages definitions happy
#include "anki/cozmo/game/comms/messaging/uiMessages.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/messaging/shared/TcpClient.h"

#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/vision/basestation/image_impl.h"
#include "anki/vision/MarkerCodeDefinitions.h"
#endif

#import "CozmoOperator.h"

// Can't this _always_ be localhost? The UI and the basestation object
// are always on the same physical device
#define BASESTATION_IP "127.0.0.1"
//#define BASESTATION_IP "192.168.19.227"
//#define BASESTATION_IP "192.168.19.238"
//#define BASESTATION_IP "192.168.18.244"

using namespace Anki;

@interface ViewController ()
{
  CvVideoCamera*            _videoCamera;
  CozmoBasestation*         _basestation;
  NSTimer*                  _connectionTimer;
  CozmoDirectionController* _directionController;
  
  // TODO: Move this somewhere else?
  Cozmo::VisionProcessingThread*  _visionThread;

}

@property (strong, nonatomic) CozmoOperator* _cozmoOperator;

@end


@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view, typically from a nib.

  // Make the head/lift sliders vertical
  [_headAngleSlider  setTransform:CGAffineTransformMakeRotation(-M_PI_2)];
  [_liftHeightSlider setTransform:CGAffineTransformMakeRotation(-M_PI_2)];
  
  // Set up videoCamera for displaying device's on-board camera, but don't start it
  _videoCamera = [[CvVideoCamera alloc] initWithParentView:_liveImageView];
  _videoCamera.delegate = self;
  _videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
  _videoCamera.defaultAVCaptureSessionPreset  = AVCaptureSessionPreset640x480;
  //_videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationLandscapeLeft;
  _videoCamera.defaultFPS = 30;
  _videoCamera.grayscaleMode = NO;
  
  _basestation = [[CozmoBasestation alloc] init];
  [_basestation startComms];
  
  // Hook up the basestation to the image viewer to start
//  _basestation._imageViewer = _liveImageView;
  _detectedMarkerLabel.hidden = YES;
  
  // Start up the vision thread for processing device camera images
  Vision::CameraCalibration deviceCamCalib;
  _visionThread = new Cozmo::VisionProcessingThread();
  _visionThread->Start(deviceCamCalib); // TODO: Only start when device camera selected?


  self._cozmoOperator = [CozmoOperator new];

  // Once done initializing, this will start the basestation timer loop
//  _firstHeartbeatTimestamp = _lastHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{

    if ([_basestation startBasestation]) {
      _connectionTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 20.0)
                                                          target:self
                                                        selector:@selector(uiHeartbeat:)
                                                        userInfo:nil
                                                         repeats:YES];
    }
  });


  NSLog(@"Finished ViewController:viewDidLoad");
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}


- (void)uiHeartbeat:(NSTimer*)timer
{
  // Connect to basestation if not already connected

//  if (_basestation.runState == CozmoBasestationRunStateCommsReady) {
//    [_basestation startBasestation];
//  }


  if (!__cozmoOperator.isConnected) {
    [__cozmoOperator connectToBasestationWithIPAddress:[NSString stringWithUTF8String:BASESTATION_IP]];
    // Set initial State
    [self actionSetHeadAngle:self.headAngleSlider];
    [self actionSetLiftHeight:self.liftHeightSlider];
  }


  [self._cozmoOperator sendWheelCommandWithAngleInDegrees:_VirtualDirectionPadView.angleInDegrees magnitude:_VirtualDirectionPadView.magnitude];

//  [self._cozmoOperator sendWheelCommandWithLeftSpeed:_VirtualDirectionPadView.leftWheelSpeed_mmps right:_VirtualDirectionPadView.rightWheelSpeed_mmps];
}


#pragma mark - UI Actions

-(void)dealloc
{

}

#pragma mark - Protocol CvVideoCameraDelegate

#ifdef __cplusplus
- (void)processImage:(cv::Mat&)imageBGR
{
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
    const char *labelCString = "No Markers";
    detectionRectColor = {0,0,255,255};
    Cozmo::MessageVisionMarker msg;
    while(true == _visionThread->CheckMailbox(msg)) {
      labelCString = Vision::MarkerTypeStrings[msg.markerType];
      detectionRectColor = {0,255,0,255};
    }
    
    // Force update of label text _now_. Using trick from here:
    // http://stackoverflow.com/questions/6835472/uilabel-text-not-being-updated
    // because this didn't seem to work:
    //   _detectedMarkerLabel.text = [[NSString alloc]initWithUTF8String:Vision::MarkerTypeStrings[msg.markerType]];
    //   [_detectedMarkerLabel setNeedsDisplay];
    dispatch_async(dispatch_get_main_queue(), ^{
      NSString *markerString = [[NSString alloc]initWithUTF8String:labelCString];
      [_detectedMarkerLabel setText:markerString];
    });
    
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
  bitwise_not(imageGray, imageGray);
  
  //Convert BGRA for display
  cvtColor(imageGray, imageBGR, COLOR_GRAY2BGRA);
  
  // Overlay the detection rectangle on the image, in the color determined above
  rectangle(imageBGR, detectRectPt1, detectRectPt2, detectionRectColor, 4);
  
}


#endif

- (IBAction)actionSetHeadAngle:(UISlider*)slider {
  [self._cozmoOperator sendHeadCommandWithAngleRatio:slider.value];
}

- (IBAction)actionSetLiftHeight:(UISlider*)slider {
  [self._cozmoOperator sendLiftCommandWithHeightRatio:slider.value];
}

- (IBAction)actionSelectCamera:(id)sender {
  
//  if(_cameraSelector.selectedSegmentIndex == 0) {
//    [_videoCamera stop];
//    _basestation._imageViewer = _liveImageView;
//    _detectedMarkerLabel.hidden = YES;
//  } else if(_cameraSelector.selectedSegmentIndex == 1) {
//    _basestation._imageViewer = nullptr;
//    [_videoCamera start];
//    _detectedMarkerLabel.hidden = NO;
//    _detectedMarkerLabel.text = @"No Markers";
//  } else {
//    NSLog(@"Error: unexpected segment index in camera selector.\n");
//  }

}
@end
