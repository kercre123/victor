//
//  ViewController.h
//  CozmoVision
//
//  Created by Andrew Stein on 11/25/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#ifdef __cplusplus
#import <opencv2/opencv.hpp>
#import <opencv2/highgui/cap_ios.h>
#endif

@interface ViewController : UIViewController<CvVideoCameraDelegate>

@property (weak, nonatomic) IBOutlet UIImageView*   liveImageView;

@property (weak, nonatomic) IBOutlet UIView*        directionControllerView;
@property (weak, nonatomic) IBOutlet UILabel*       directionOLabel;

@property (weak, nonatomic) IBOutlet UILabel*       upArrowLabel;
@property (weak, nonatomic) IBOutlet UILabel*       downArrayLabel;

@property (weak, nonatomic) IBOutlet UISlider*      headAngleSlider;
- (IBAction)actionSetHeadAngle:(id)sender;

@property (weak, nonatomic) IBOutlet UISlider*      liftHeightSlider;
- (IBAction)actionSetLiftHeight:(id)sender;

@property (weak, nonatomic) IBOutlet UISegmentedControl* cameraSelector;
- (IBAction)actionSelectCamera:(id)sender;

@property (weak, nonatomic) IBOutlet UILabel*       detectedMarkerLabel;

@end

