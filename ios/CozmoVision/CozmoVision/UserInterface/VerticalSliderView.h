//
//  VerticalSliderView.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/16/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface VerticalSliderView : UIView
// Title width can be larger then view's frame
@property (copy, nonatomic) NSString* title;
// The value of allowd value change before action block is called [0.0 - 1.0]
@property (assign, nonatomic) float valueChangedThreshold;
// Value change action call back block
@property (copy, nonatomic) void(^actionBlock)(float value);
// Current Value
@property (readonly) float sliderValue;
// Update slider position
- (void)setValue:(float)newValue;
// Set limits
- (void)setValueRange:(float)minValue
                     :(float)maxValue;
// Decrements the lockout timer
// (Call this every UI heartbeat)
-(void)decrementLockoutTimer;
@end
