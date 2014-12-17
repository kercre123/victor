//
//  VerticalSliderView.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/16/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "VerticalSliderView.h"

#define kMinViewWidth       44.0
#define kNoSliderValue      NAN
@interface VerticalSliderView ()

@property (strong, nonatomic) UILabel* titleLabel;
@property (strong, nonatomic) UILabel* valueLabel;
@property (strong, nonatomic) UISlider* slider;

@property (assign, nonatomic) float _touchDownSliderValue;

@end

@implementation VerticalSliderView


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
  self.backgroundColor = [UIColor clearColor];

  self.titleLabel = [UILabel new];
  self.titleLabel.text = @"title";
  self.titleLabel.textColor = self.tintColor;
  self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.titleLabel];

  self.valueLabel = [UILabel new];
  self.valueLabel.text = @"0%";
  self.valueLabel.textColor = self.tintColor;
  self.valueLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.valueLabel];

  self.slider = [UISlider new];
  [self.slider addTarget:self action:@selector(handleSliderTouchDownEvent:) forControlEvents:UIControlEventTouchDown];
  [self.slider addTarget:self action:@selector(handleSliderTouchUpEvent:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchDragOutside];
  [self.slider addTarget:self action:@selector(handleSliderValueChange:) forControlEvents:UIControlEventValueChanged];
  self.slider.transform = CGAffineTransformMakeRotation(-M_PI_2);
  [self addSubview:self.slider];

  self._touchDownSliderValue = kNoSliderValue;

  // Auto Layout
  NSDictionary *bindings = @{ @"titleLabel" : self.titleLabel, @"valueLabel" : self.valueLabel, @"slider" : self.slider };

  [self addConstraint:[NSLayoutConstraint constraintWithItem:self attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationGreaterThanOrEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1.0 constant:kMinViewWidth]];

  [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[titleLabel]-(>=20)-[valueLabel]|" options:0 metrics:nil views:bindings]];

  [self addConstraint:[NSLayoutConstraint constraintWithItem:self.titleLabel attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];

  [self addConstraint:[NSLayoutConstraint constraintWithItem:self.valueLabel attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  // Manually set slider frame
  CGSize viewSize = self.bounds.size;
  CGFloat topBottomPadding = 0.0;
  CGFloat height = viewSize.height - self.titleLabel.bounds.size.height - self.valueLabel.bounds.size.height - topBottomPadding - topBottomPadding;
  self.slider.bounds = CGRectMake(0.0, 0.0, height, kMinViewWidth);
  self.slider.center = CGPointMake(viewSize.width / 2.0, viewSize.height / 2.0);
}

- (void)setTintColor:(UIColor *)tintColor
{
  [super setTintColor:tintColor];
  self.titleLabel.textColor = self.tintColor;
  self.valueLabel.textColor = self.tintColor;
}

#pragma mark - Slider Action Methods

- (void)handleSliderTouchDownEvent:(UISlider*)slider
{
//  NSLog(@"handleSliderTouchDownEvent");

  self._touchDownSliderValue = slider.value;
}

- (void)handleSliderTouchUpEvent:(UISlider*)slider
{
//  NSLog(@"handleSliderTouchUpEvent");
  self._touchDownSliderValue = kNoSliderValue;

  // Send last value
  if (self.actionBlock) {
    self.actionBlock(slider.value);
  }
}

- (void)handleSliderValueChange:(UISlider*)slider
{
//  NSLog(@"handleSliderValueChange Threshold %@ - %@", (_valueChangedThreshold != kNoSliderValue && ABS(self._touchDownSliderValue - slider.value) > _valueChangedThreshold) ? @"Y" : @"N", slider);

  // Display value
  float sliderValue = slider.value;
  float percentVal = sliderValue * 100.0;
  self.valueLabel.text = [NSString stringWithFormat:@"%.0f%%", percentVal];

  // Check Threshold
  float diffVal = (_valueChangedThreshold != kNoSliderValue) ? ABS(self._touchDownSliderValue - sliderValue) : 0.0;
  BOOL passedThreshold = diffVal > _valueChangedThreshold;

//  NSLog(@"handleSliderValueChange val %f dif %f threshold %s", sliderValue, diffVal, passedThreshold ? "Y" : "N" );

  if (self.actionBlock && passedThreshold ) {
    self.actionBlock(sliderValue);
    self._touchDownSliderValue = sliderValue;
  }
}


#pragma mark - Public methods

- (NSString *)title
{
  return self.titleLabel.text;
}

- (void)setTitle:(NSString*)title
{
  self.titleLabel.text = title;
}

- (float)sliderValue
{
  return self.slider.value;
}



@end
