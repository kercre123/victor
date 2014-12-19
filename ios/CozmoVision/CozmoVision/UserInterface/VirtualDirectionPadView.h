//
//  VirtualDirectionPadView.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/10/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface VirtualDirectionPadView : UIView

#define kDefaultMagnitudeRadius     140.0
#define kDefaultPuckRadius          30.0
#define kDefaultAngleThreshold      5.0
#define kDefaultMagnitudeThreshold  0.10

// Config Direction Pad

// Radius of magnitude in points
@property (assign, nonatomic) CGFloat magnitudeRadius;
// Radius of thumb puck in points
@property (assign, nonatomic) CGFloat thumbPuckRadius;

// Angle Threshold value is in Degrees
@property (readwrite, nonatomic) CGFloat angleThreshold;
// Magnitude Threshold value is in ratio [0.0 - 1.0]
@property (readwrite, nonatomic) CGFloat magnitudeThreshold;

// Action call back when the joystick movment value passes either angle or magnitude threshold
@property (copy, nonatomic) void(^joystickMovementAction)(CGFloat angleInDegrees, CGFloat magnitude);

// Action call back when view is doubled tapped
@property (copy, nonatomic) void(^doubleTapAction)(UIGestureRecognizer* gesture);

// Check if Direction Pad is active
@property (readonly) BOOL    isActivelyControlling;





@property (readonly) BOOL isDirty;  // Joystick vaule hadn't changed over threshold
- (void)clearDirtyFlag; // Reset the dirty flag

// Current
@property (readonly) CGFloat angleInDegrees;
@property (readonly) CGFloat magnitude;
@property (readonly) CGPoint point;// Debug purposses


// TODO: Remove wheel speeds
@property (readonly) CGFloat leftWheelSpeed_mmps; // Left wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop
@property (readonly) CGFloat rightWheelSpeed_mmps; // Right wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop

@end
