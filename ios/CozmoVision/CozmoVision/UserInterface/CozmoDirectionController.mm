//
//  CozmoDirectionController.m
//  CozmoVision
//
//  Created by Andrew Stein on 12/8/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "CozmoDirectionController.h"

#import <anki/common/types.h>
#import <anki/cozmo/shared/cozmoConfig.h>

#define DEBUG_GAMEPAD 0

@implementation CozmoDirectionController

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self) {
    // Initialization code
 
  }
  return self;
}

-(void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  [super touchesBegan:touches withEvent:event];
  [self updateSpeedsWithTouches:touches];
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  [super touchesMoved:touches withEvent:event];
  [self updateSpeedsWithTouches:touches];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  [super touchesEnded:touches withEvent:event];
  _leftWheelSpeed_mmps  = 0;
  _rightWheelSpeed_mmps = 0;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  [super touchesCancelled:touches withEvent:event];
  _leftWheelSpeed_mmps  = 0;
  _rightWheelSpeed_mmps = 0;
}

- (void)updateSpeedsWithTouches:(NSSet *)touches
{
  UITouch *touch = [touches anyObject];
  CGPoint touchPoint = [touch locationInView:touch.view];
  
  CGFloat midX  = CGRectGetMidX(touch.view.bounds);
  CGFloat width = 0.5f*CGRectGetWidth(touch.view.bounds);
  
  CGFloat midY   = CGRectGetMidY(touch.view.bounds);
  CGFloat height = 0.5f*CGRectGetHeight(touch.view.bounds);
  
  CGPoint centerOffset = CGPointMake((touchPoint.x - midX)/width,
                                     (touchPoint.y - midY)/height);
  
  [self calculateWheelSpeeds:centerOffset];
}

- (void)calculateWheelSpeeds:(CGPoint)point
{
  const f32 ANALOG_INPUT_DEAD_ZONE_THRESH = .1f; //000.f / f32(s16_MAX);
  const f32 ANALOG_INPUT_MAX_DRIVE_SPEED  = 100; // mm/s
  const f32 MAX_ANALOG_RADIUS             = 300;
  
#if(DEBUG_GAMEPAD)
  printf("AnalogLeft X %.2f  Y %.2f\n", point.x, point.y);
#endif
  
  //point.x = MAX(point.x, ANALOG_INPUT_DEAD_ZONE_THRESH);
  //point.y = MAX(point.y, ANALOG_INPUT_DEAD_ZONE_THRESH);
  
  // Compute speed
  CGFloat xyMag = MIN(1.0, sqrtf( point.x*point.x + point.y*point.y));
  
  _leftWheelSpeed_mmps = 0;
  _rightWheelSpeed_mmps = 0;
  
  // Stop wheels if magnitude of input is low
  if (xyMag > ANALOG_INPUT_DEAD_ZONE_THRESH) {

    xyMag -= ANALOG_INPUT_DEAD_ZONE_THRESH;
    xyMag /= 1.f - ANALOG_INPUT_DEAD_ZONE_THRESH;
  
    // Driving forward?
    f32 fwd = point.y < 0 ? 1 : -1;
    
    // Curving right?
    f32 right = point.x > 0 ? 1 : -1;
    
    // Base wheel speed based on magnitude of input and whether or not robot is driving forward
    f32 baseWheelSpeed = ANALOG_INPUT_MAX_DRIVE_SPEED * xyMag * fwd;
    
    
    // Angle of xy input used to determine curvature
    f32 xyAngle = fabsf(atanf(-(f32)point.y/ (f32)point.x)) * (right);
    
    // Compute radius of curvature
    f32 roc = (xyAngle / PIDIV2_F) * MAX_ANALOG_RADIUS;
    
    
    // Compute individual wheel speeds
    if (fabsf(xyAngle) > PIDIV2_F - 0.1f) {
      // Straight fwd/back
      _leftWheelSpeed_mmps  = baseWheelSpeed;
      _rightWheelSpeed_mmps = baseWheelSpeed;
    } else if (fabsf(xyAngle) < 0.1f) {
      // Turn in place
      _leftWheelSpeed_mmps  = right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
      _rightWheelSpeed_mmps = -right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
    } else {
      
      _leftWheelSpeed_mmps = baseWheelSpeed * (roc + (right * Anki::Cozmo::WHEEL_DIST_HALF_MM)) / roc;
      _rightWheelSpeed_mmps = baseWheelSpeed * (roc - (right * Anki::Cozmo::WHEEL_DIST_HALF_MM)) / roc;
      
      // Swap wheel speeds
      if (fwd * right < 0) {
        f32 temp = _leftWheelSpeed_mmps;
        _leftWheelSpeed_mmps = _rightWheelSpeed_mmps;
        _rightWheelSpeed_mmps = temp;
      }
    }
    
    
    // Cap wheel speed at max
    if (fabsf(_leftWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const f32 correction = _leftWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_leftWheelSpeed_mmps >= 0 ? 1 : -1));
      const f32 correctionFactor = 1.f - fabsf(correction / _leftWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("lcorrectionFactor: %f\n", correctionFactor);
    }
    if (fabsf(_rightWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const f32 correction = _rightWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_rightWheelSpeed_mmps >= 0 ? 1 : -1));
      const f32 correctionFactor = 1.f - fabsf(correction / _rightWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("rcorrectionFactor: %f\n", correctionFactor);
    }
    
#if(DEBUG_GAMEPAD)
    printf("AnalogLeft: xyMag %f, xyAngle %f, radius %f, fwd %f, right %f, lwheel %f, rwheel %f\n", xyMag, xyAngle, roc, fwd, right, _leftWheelSpeed_mmps, _rightWheelSpeed_mmps );
#endif
  
  } // if sufficient speed

} //calculateWheelSpeeds()

@end
