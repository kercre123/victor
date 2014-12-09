//
//  ViewController.m
//  CozmoVision
//
//  Created by Andrew Stein on 11/25/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "ViewController.h"

#import <CoreGraphics/CoreGraphics.h>

#import "CozmoBasestation.h"
#import "CozmoDirectionController.h"

#ifdef __cplusplus
// For OpenCV Video processing of phone's camera:
#import <opencv2/opencv.hpp>
#import <opencv2/highgui/cap_ios.h>

// For communicating UI Messages
#define COZMO_BASESTATION // to make uiMessages definitions happy
#include "anki/cozmo/basestation/ui/messaging/uiMessages.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/messaging/shared/TcpClient.h"
#endif

// Can't this _always_ be localhost? The UI and the basestation object
// are always on the same physical device
//#define BASESTATION_IP "127.0.0.1"
#define BASESTATION_IP "192.168.42.68"

using namespace Anki;

@interface ViewController ()
{
  CvVideoCamera*            _videoCamera;
  CozmoBasestation*         _basestation;
  TcpClient*                _uiClient; // Why isn't this UiTcpComms?
  BOOL                      _isConnected;
  NSTimer*                  _connectionTimer;
  CozmoDirectionController* _directionController;
}

// UI Message Sending
-(void)SendDriveWheels:(f32)lwheel_speed_mmps  :(f32)rwheel_speed_mmps;

-(void)SendMessage:(Cozmo::UiMessage&) msg;

@end


@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view, typically from a nib.
  
  // Rotate the up/down arrow labels:
  [_upArrowLabel   setTransform:CGAffineTransformMakeRotation(-M_PI_2)];
  [_downArrayLabel setTransform:CGAffineTransformMakeRotation( M_PI_2)];
  [_upArrowLabel   bringSubviewToFront:_directionControllerView];
  
  // Make sure "O" in background of D-pad is at the back
  [_directionOLabel sendSubviewToBack:_directionControllerView];
  
  // Make the head/lift sliders vertical
  [_headAngleSlider  setTransform:CGAffineTransformMakeRotation(-M_PI_2)];
  [_liftHeightSlider setTransform:CGAffineTransformMakeRotation(-M_PI_2)];
  
  // Set up videoCamera for displaying device's on-board camera, but don't start it
  _videoCamera = [[CvVideoCamera alloc] initWithParentView:_robotImageView];
  _videoCamera.delegate = self;
  _videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
  _videoCamera.defaultAVCaptureSessionPreset  = AVCaptureSessionPreset352x288;
  _videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationLandscapeLeft;
  _videoCamera.defaultFPS = 30;
  _videoCamera.grayscaleMode = NO;
  
  _basestation = [[CozmoBasestation alloc] init];
  
  // Hook up the basestation to the image viewer to start
  _basestation._imageViewer = _robotImageView;
  
  _directionController = [[CozmoDirectionController alloc] initWithFrame:_directionControllerView.bounds];
  [_directionControllerView addSubview:_directionController];
  
#ifdef __cplusplus
  _uiClient = new TcpClient();
#endif
  _isConnected = NO;
  
  // Once done initializing, this will start the basestation timer loop
  //_firstHeartbeatTimestamp = _lastHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  _connectionTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / FRAME_RATE)
                                                     target:self
                                                   selector:@selector(uiHeartbeat:)
                                                   userInfo:nil
                                                    repeats:YES];
  
  NSLog(@"Finished ViewController:viewDidLoad");
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}


- (void)uiHeartbeat:(NSTimer*)timer
{
  // Connect to basestation if not already connected
  if (!_uiClient->IsConnected()) {
    if (!_uiClient->Connect(BASESTATION_IP, Cozmo::UI_MESSAGE_SERVER_LISTEN_PORT)) {
      NSLog(@"Failed to connect to UI message server listen port\n");
      _isConnected = NO;
      return;
    }
    NSLog(@"Cozmo iOS UI connected to basestation!\n");
    _isConnected = YES;
  }

  
  //NSLog(@"LeftWheel = %.2f, RightWheel = %.f\n",
  //      _directionController.leftWheelSpeed_mmps,
  //      _directionController.rightWheelSpeed_mmps);
  
  [self SendDriveWheels:_directionController.leftWheelSpeed_mmps
                       :_directionController.rightWheelSpeed_mmps];
  
}


#pragma mark - UI Actions

-(void)dealloc
{
  
  if(_uiClient) {
    delete _uiClient;
    _uiClient = nullptr;
  }
}

#pragma mark - Protocol CvVideoCameraDelegate

#ifdef __cplusplus
- (void)processImage:(cv::Mat&)image
{
  
  using namespace cv;
  
  // Do some OpenCV stuff with the image
  Mat image_copy;
  cvtColor(image, image_copy, COLOR_BGR2GRAY);
  
  // invert image
  bitwise_not(image_copy, image_copy);
  
  //Convert BGR to BGRA (three channel to four channel)
  Mat bgr;
  cvtColor(image_copy, bgr, COLOR_GRAY2BGR);
  
  cvtColor(bgr, image, COLOR_BGR2BGRA);
  
  rectangle(image, Point2i(126,94), Point2i(226,194), Scalar(0,0,255,255), 2);
}


- (void) SendMessage:(Cozmo::UiMessage&)msg
{
  if(_isConnected) {
    // TODO: Put this stuff in init so we don't have to do it every time?
    char sendBuf[128];
    memcpy(sendBuf, Cozmo::RADIO_PACKET_HEADER, sizeof(Cozmo::RADIO_PACKET_HEADER));
    
    int sendBufLen = sizeof(Cozmo::RADIO_PACKET_HEADER);
    sendBuf[sendBufLen++] = msg.GetSize()+1;
    sendBuf[sendBufLen++] = msg.GetID();
    msg.GetBytes((u8*)(&sendBuf[sendBufLen]));
    sendBufLen += msg.GetSize();
    
    //int bytes_sent =
    _uiClient->Send(sendBuf, sendBufLen);
    //printf("Sent %d bytes\n", bytes_sent);
    } // if(_isConnected)
}


- (void) SendDriveWheels:(f32)lwheel_speed_mmps :(f32)rwheel_speed_mmps
{
  Cozmo::MessageU2G_DriveWheels m;
  m.lwheel_speed_mmps = lwheel_speed_mmps;
  m.rwheel_speed_mmps = rwheel_speed_mmps;
  [self SendMessage:m];
}

#endif

- (IBAction)actionSetHeadAngle:(id)sender {
  
  Cozmo::MessageU2G_SetHeadAngle m;
  
  m.angle_rad = ABS(_headAngleSlider.value) * (_headAngleSlider.value < 0 ?
                                               Cozmo::MIN_HEAD_ANGLE  :
                                               Cozmo::MAX_HEAD_ANGLE);
  
  m.accel_rad_per_sec2    = 2.f;
  m.max_speed_rad_per_sec = 5.f;
  
  [self SendMessage:m];
}

- (IBAction)actionSetLiftHeight:(id)sender {
  
  Cozmo::MessageU2G_SetLiftHeight m;
  
  m.height_mm = _liftHeightSlider.value * (Cozmo::LIFT_HEIGHT_CARRY - Cozmo::LIFT_HEIGHT_LOWDOCK) + Cozmo::LIFT_HEIGHT_LOWDOCK;

  m.accel_rad_per_sec2    = 2.f;
  m.max_speed_rad_per_sec = 5.f;

  [self SendMessage:m];
}

- (IBAction)actionSelectCamera:(id)sender {
  
  if(_cameraSelector.selectedSegmentIndex == 0) {
    [_videoCamera stop];
    _basestation._imageViewer = _robotImageView;
  } else if(_cameraSelector.selectedSegmentIndex == 1) {
    _basestation._imageViewer = nullptr;
    [_videoCamera start];
  } else {
    NSLog(@"Error: unexpected segment index in camera selector.\n");
  }
  
}
@end
