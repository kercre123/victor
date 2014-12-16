//
//  DirectionPadView.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/10/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DirectionPadView : UIView

@property (readwrite, nonatomic) CGFloat angleThreshold; // Value Threshold is in points
@property (readwrite, nonatomic) CGFloat magmitudeThreshold; // Value Threshold is in points
@property (readonly) BOOL isDirty;  // Joystick vaule hadn't changed over threshold
- (void)clearDirtyFlag; // Reset the dirty flag

@property (readonly) CGFloat angleInDegrees;
@property (readonly) CGFloat magnitude;
@property (readonly) CGPoint point;// Debug purposses

// TODO: Remove wheel speeds
@property (readonly) CGFloat leftWheelSpeed_mmps; // Left wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop
@property (readonly) CGFloat rightWheelSpeed_mmps; // Right wheel speed (1.0, -1.0) == (Foward, Reverse), 0.0 = Stop

@end
