//
//  CvVideoCameraReorient.h
//  CozmoVision
//
//  Modified CvVideoCameraDelegate to handle orientation change more nicely.
//  See here:
//     http://answers.opencv.org/question/8329/opencv-setting-ios-orientation/
//
//  Created by Andrew Stein on 1/8/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import <opencv2/highgui/cap_ios.h>

@protocol CvVideoCameraDelegateReorient <CvVideoCameraDelegate>
@end

@interface CvVideoCameraReorient : CvVideoCamera

- (void)updateOrientation;
- (void)layoutPreviewLayer;

@end