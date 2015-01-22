//
//  VirtualDirectionPadView.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/10/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "VirtualDirectionPadView.h"

// The number of heartbeats that must pass before the next dpad
// message can be sent. Prevents spamming in the event of a fast changing dpad input.
#define DPAD_LOCKOUT_COUNT 1

@interface VirtualDirectionPadView ()

// Public properties
@property (readwrite, nonatomic) BOOL isDirty;
@property (readwrite, nonatomic) int lockoutTimer;

@property (readwrite, nonatomic) CGFloat angleInDegrees;
@property (readwrite, nonatomic) CGFloat magnitude;
@property (readwrite, nonatomic) BOOL    isActivelyControlling;

@property (readwrite, nonatomic) CGFloat leftWheelSpeed_mmps;
@property (readwrite, nonatomic) CGFloat rightWheelSpeed_mmps;

// Direction Pad Gesture
@property (strong, nonatomic) UIPanGestureRecognizer* panGesture;
@property (strong, nonatomic) UITapGestureRecognizer* tapGesture;

// View objects
@property (strong, nonatomic) UIView* joystickView;
@property (strong, nonatomic) UIView* thumbPuckView;


@property (assign, nonatomic) CGFloat _previousAngle;
@property (assign, nonatomic) CGFloat _previousMagnitude;

@end


@implementation VirtualDirectionPadView

- (instancetype)init
{
  if (!(self = [super init]))
    return self;

  [self commonInit];

  return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (!(self = [super initWithFrame:frame]))
    return self;

  [self commonInit];

  return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
  if (!(self = [super initWithCoder:aDecoder]))
    return self;

  [self commonInit];

  return self;
}

- (void)commonInit
{
  self.lockoutTimer = 0;
  self.backgroundColor = [UIColor clearColor];
  self.magnitudeRadius = kDefaultMagnitudeRadius;
  self.thumbPuckRadius = kDefaultPuckRadius;
  self.angleThreshold = kDefaultAngleThreshold;
  self.magnitudeThreshold = kDefaultMagnitudeThreshold;

  // Create Background view
  self.joystickView = [[UIView alloc] initWithFrame:CGRectMake(0.0, 0.0, self.magnitudeRadius, self.magnitudeRadius)];
  self.joystickView.backgroundColor = [UIColor whiteColor];
  self.joystickView.layer.cornerRadius = self.magnitudeRadius / 2.0;
  self.joystickView.layer.borderColor = [UIColor blackColor].CGColor;
  self.joystickView.layer.borderWidth = 1.0;
  self.joystickView.layer.allowsGroupOpacity = YES;
  [self addSubview:self.joystickView];


  // Creat Direction Arrows Image Views for background view
  UIImage* upDownArrowImage = [UIImage imageNamed:@"DirectionArrowUp"];
  UIImage* leftRightArrowImage = [UIImage imageWithCGImage:upDownArrowImage.CGImage scale:upDownArrowImage.scale orientation:UIImageOrientationLeft];

  UIImageView *upArrow = [[UIImageView alloc] initWithImage:upDownArrowImage];
  upArrow.translatesAutoresizingMaskIntoConstraints = NO;
  [self.joystickView addSubview:upArrow];

  UIImageView *downArrow = [[UIImageView alloc] initWithImage:upDownArrowImage];
  downArrow.transform = CGAffineTransformMakeRotation(M_PI);
  downArrow.translatesAutoresizingMaskIntoConstraints = NO;
  [self.joystickView addSubview:downArrow];

  UIImageView *leftArrow = [[UIImageView alloc] initWithImage:leftRightArrowImage];
  leftArrow.translatesAutoresizingMaskIntoConstraints = NO;
  [self.joystickView addSubview:leftArrow];

  UIImageView *rightArrow = [[UIImageView alloc] initWithImage:leftRightArrowImage];
  rightArrow.transform = CGAffineTransformMakeRotation(M_PI);
  rightArrow.translatesAutoresizingMaskIntoConstraints = NO;
  [self.joystickView addSubview:rightArrow];

  // Layout out Arrow IVs using Constraints
  CGFloat arrowMargin = 4.0;
  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:upArrow attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeTop multiplier:1.0 constant:arrowMargin]];
  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:upArrow attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];

  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:downArrow attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-arrowMargin]];
  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:downArrow attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];

  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:leftArrow attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeLeft multiplier:1.0 constant:arrowMargin]];
  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:leftArrow attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0]];

  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:rightArrow attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeRight multiplier:1.0 constant:-arrowMargin]];
  [self.joystickView addConstraint:[NSLayoutConstraint constraintWithItem:rightArrow attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self.joystickView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0]];



  self.thumbPuckView = [[UIView alloc] initWithFrame:CGRectMake(0.0, 0.0, self.thumbPuckRadius, self.thumbPuckRadius)];
  self.thumbPuckView.backgroundColor = [UIColor redColor];
  self.thumbPuckView.layer.cornerRadius = self.thumbPuckRadius / 2.0;
  self.thumbPuckView.layer.borderColor = [UIColor colorWithWhite:0.4 alpha:1.0].CGColor;
  self.thumbPuckView.layer.borderWidth = 1.0;
  // Set size in -layoutSubviews
  [self addSubview:self.thumbPuckView];

  
  self.tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
  self.tapGesture.cancelsTouchesInView = NO;
  self.tapGesture.numberOfTapsRequired = 2; // look for double-tap
  [self addGestureRecognizer:self.tapGesture];

  self.panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
  self.panGesture.cancelsTouchesInView = NO;
  [self addGestureRecognizer:self.panGesture];

  
  // Initial State
  [self showJoystickView:NO animated:NO];
  
}

- (void)showJoystickView:(BOOL)show animated:(BOOL)animated
{
  CGFloat joystickAlpha = show ? 0.3 : 0.0;
  CGFloat thumbPuckAlpha = show ? 1.0 : 0.0;

  if (animated) {
    [UIView animateWithDuration:0.2 animations:^{
      self.joystickView.alpha = joystickAlpha;
      self.thumbPuckView.alpha = thumbPuckAlpha;
    }];
  }
  else {
    self.joystickView.alpha = joystickAlpha;
    self.thumbPuckView.alpha = thumbPuckAlpha;
  }

}


#pragma mark - Pan Guesture Methods

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
  // Allow both gesture recognizers (pan and tap) to work simultaneously!
  return YES;
}

- (void)handlePanGesture:(UIPanGestureRecognizer*)gesture
{
  CGPoint superViewPoint = [gesture locationInView:self];
  CGPoint point = [gesture locationInView:self.joystickView];

//  [self updateJoystickPositionWithPoint:point];

  // Update Joystick

//  NSLog(@"handlePanGesture: %@, point X:%f Y: %f", gesture, point.x, point.y);

  switch (gesture.state) {
    case UIGestureRecognizerStateBegan:
    {
      // Set initial Position
      self.joystickView.center = superViewPoint;
      self.thumbPuckView.center = superViewPoint;
      [self showJoystickView:YES animated:YES];
      self._previousAngle = 0.0;
      self._previousMagnitude = 0.0;
//      [self updateJoystickPositionWithPoint:point];
//      self._previousAngle = self.angleInDegrees;
//      self._previousMagnitude = self.magnitude;
      self.isActivelyControlling = YES;
    }
      break;

    case UIGestureRecognizerStateChanged:
      self.thumbPuckView.center = superViewPoint;
      [self updateJoystickPositionWithPoint:point];
      self.isActivelyControlling = YES;
      break;

    case UIGestureRecognizerStateEnded:
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed:
    {
      // TODO: TEMP Solution to Stop robot
      self.angleInDegrees = 0.0;
      self.magnitude = 0.0;
      self.leftWheelSpeed_mmps = 0.0;
      self.rightWheelSpeed_mmps = 0.0;
      self.isActivelyControlling = NO;

      [self showJoystickView:NO animated:YES];
    }
      break;
    case UIGestureRecognizerStatePossible:

      break;

    default:
      break;
  }

}


-(void)decrementLockoutTimer
{
  if (self.lockoutTimer > 0)
    self.lockoutTimer--;
}


- (void)clearDirtyFlag
{
  // Clear flag set current point as last point
  
}

- (void)handleTapGesture:(UITapGestureRecognizer*)gesture
{
  if(self.doubleTapAction) {
    self.doubleTapAction(gesture);
  }
}

#pragma Calculate stuff

// TODO: Need to fix this to work within the bounds that the joystick can move in
// TODO: Need to add angle & magnitude thresholds
- (void)updateJoystickPositionWithPoint:(CGPoint)point
{
  CGFloat midX  = CGRectGetMidX(self.joystickView.bounds);
  CGFloat width = 0.5f*CGRectGetWidth(self.joystickView.bounds);

  CGFloat midY   = CGRectGetMidY(self.joystickView.bounds);
  CGFloat height = 0.5f*CGRectGetHeight(self.joystickView.bounds);

  CGPoint centerOffset = CGPointMake((point.x - midX)/width,
                                     (point.y - midY)/height);

//  NSLog(@"updateJoystickPositionWithPoint offset x:%f y:%f", centerOffset.x, centerOffset.y);

  [self calculateAngleAndMagnitude:centerOffset];

  [self calculateWheelSpeeds:centerOffset];
}

- (void)calculateAngleAndMagnitude:(CGPoint)point
{
  // Compute speed
  CGFloat pointX = MIN(point.x, _magnitudeRadius);
  CGFloat pointY = MIN(point.y, _magnitudeRadius);

  _magnitude = MIN(1.0, sqrt( (double)(pointX * pointX) + (double)(pointY * pointY) ));

  const CGFloat oneEightyOverPi = 180.0 / M_PI;

  CGFloat angleInRads = atan2f(-pointY, pointX);
  _angleInDegrees = angleInRads * oneEightyOverPi;

  if (!self.lockoutTimer && (ABS(__previousAngle - _angleInDegrees) > _angleThreshold || ABS(__previousMagnitude - _magnitude) > _magnitudeThreshold)) {
    __previousAngle = _angleInDegrees;
    __previousMagnitude = _magnitude;
    if (self.joystickMovementAction) {
      self.joystickMovementAction(_angleInDegrees, _magnitude);
      self.lockoutTimer = DPAD_LOCKOUT_COUNT;
    }
  }
  }
}

- (void)calculateWheelSpeeds:(CGPoint)point
{

  _point = point;

  const float ANALOG_INPUT_DEAD_ZONE_THRESH = .1f; //000.f / float(s16_MAX);
  const float ANALOG_INPUT_MAX_DRIVE_SPEED  = 100; // mm/s
  const float MAX_ANALOG_RADIUS             = 300;

#if(DEBUG_GAMEPAD)
  printf("AnalogLeft X %.2f  Y %.2f\n", point.x, point.y);
#endif

  // Compute speed
  CGFloat xyMag = MIN(1.0, sqrtf( point.x*point.x + point.y*point.y));

  _leftWheelSpeed_mmps = 0;
  _rightWheelSpeed_mmps = 0;

  // Stop wheels if magnitude of input is low
  if (xyMag > ANALOG_INPUT_DEAD_ZONE_THRESH) {

    xyMag -= ANALOG_INPUT_DEAD_ZONE_THRESH;
    xyMag /= 1.f - ANALOG_INPUT_DEAD_ZONE_THRESH;

    // Driving forward?
    float fwd = point.y < 0 ? 1 : -1;

    // Curving right?
    float right = point.x > 0 ? 1 : -1;

    // Base wheel speed based on magnitude of input and whether or not robot is driving forward
    float baseWheelSpeed = ANALOG_INPUT_MAX_DRIVE_SPEED * xyMag * fwd;


    // Angle of xy input used to determine curvature
    float xyAngle = fabsf(atanf(-(float)point.y/ (float)point.x)) * (right);

    // Compute radius of curvature
    float roc = (xyAngle / M_PI_2) * MAX_ANALOG_RADIUS;


    // Compute individual wheel speeds
    if (fabsf(xyAngle) > M_PI_2 - 0.1f) {
      // Straight fwd/back
      _leftWheelSpeed_mmps  = baseWheelSpeed;
      _rightWheelSpeed_mmps = baseWheelSpeed;
    } else if (fabsf(xyAngle) < 0.1f) {
      // Turn in place
      _leftWheelSpeed_mmps  = right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
      _rightWheelSpeed_mmps = -right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
    } else {

      const CGFloat wheelDistHalfMM = 47.7f / 2.0; // distance b/w the front wheels
      _leftWheelSpeed_mmps = baseWheelSpeed * (roc + (right * wheelDistHalfMM)) / roc;
      _rightWheelSpeed_mmps = baseWheelSpeed * (roc - (right * wheelDistHalfMM)) / roc;

      // Swap wheel speeds
      if (fwd * right < 0) {
        float temp = _leftWheelSpeed_mmps;
        _leftWheelSpeed_mmps = _rightWheelSpeed_mmps;
        _rightWheelSpeed_mmps = temp;
      }
    }


    // Cap wheel speed at max
    if (fabsf(_leftWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const float correction = _leftWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_leftWheelSpeed_mmps >= 0 ? 1 : -1));
      const float correctionFactor = 1.f - fabsf(correction / _leftWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("lcorrectionFactor: %f\n", correctionFactor);
    }
    if (fabsf(_rightWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const float correction = _rightWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_rightWheelSpeed_mmps >= 0 ? 1 : -1));
      const float correctionFactor = 1.f - fabsf(correction / _rightWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("rcorrectionFactor: %f\n", correctionFactor);
    }

//#if(DEBUG_GAMEPAD)
//    printf("AnalogLeft: xyMag %f, xyAngle %f, radius %f, fwd %f, right %f, lwheel %f, rwheel %f\n", xyMag, xyAngle, roc, fwd, right, _leftWheelSpeed_mmps, _rightWheelSpeed_mmps );
//#endif

  } // if sufficient speed

} //calculateWheelSpeeds()

@end
