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

// Radius of magnitude in points
@property (assign, nonatomic) CGFloat magnitudeRadius;
// Radius of thumb puck in points
@property (assign, nonatomic) CGFloat thumbPuckRadius;

@property (copy, nonatomic) void(^doubleTapAction)(UIGestureRecognizer*);

@property (readwrite, nonatomic) CGFloat angleThreshold; // Value Threshold is in points
@property (readwrite, nonatomic) CGFloat magmitudeThreshold; // Value Threshold is in points
@property (readonly) BOOL isDirty;  // Joystick vaule hadn't changed over threshold
- (void)clearDirtyFlag; // Reset the dirty flag

@property (readonly) CGFloat angleInDegrees;
@property (readonly) CGFloat magnitude;
@property (readonly) CGPoint point;// Debug purposses

@property (readonly) BOOL    isActivelyControlling;

// TODO: Remove wheel speeds
@property (readonly) CGFloat leftWheelSpeed_mmps; // Left wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop
@property (readonly) CGFloat rightWheelSpeed_mmps; // Right wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop

@end
