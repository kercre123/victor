//
//  BulletOverlayView.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "BulletOverlayView.h"

#define kFrameInterval   2

@interface BulletOverlayView ()
@end

@implementation BulletOverlayView

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

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
  self.userInteractionEnabled = NO;
}



#pragma mark - Public Methods

- (void)animateShotWithBullet:(BulletObject *)bullet
{
  CABasicAnimation* animation = [CABasicAnimation animationWithKeyPath:@"position"];
  animation.fromValue = [NSValue valueWithCGPoint:bullet.startPoint];
  animation.toValue = [NSValue valueWithCGPoint:bullet.endPoint];
  animation.duration = bullet.duration;
  [animation setFillMode:kCAFillModeRemoved];
  animation.delegate = self;
  [animation setValue:bullet.bulletLayer forKey:@"animationLayer"];

  bullet.bulletLayer.position = bullet.endPoint;
  [bullet.bulletLayer addAnimation:animation forKey:@"position"];
  [self.layer addSublayer:bullet.bulletLayer];
}


@end



@implementation BulletOverlayView (bulletFactory)

- (void)fireLaserButtlet
{
  CGPoint center = CGPointMake(self.bounds.size.width / 2.0, self.bounds.size.height / 2.0);
  UIImage *laserImage = [UIImage imageNamed:@"LaserBullet"];

  // Left Bullet
  BulletObject* leftBullet = [BulletObject new];
  leftBullet.startPoint = CGPointMake(0.0, center.y - 160.0);
  leftBullet.endPoint = center;
  leftBullet.duration = 0.2;

  CALayer* leftbulletLayer = [CALayer layer];
  leftbulletLayer.backgroundColor = [UIColor clearColor].CGColor;
  leftbulletLayer.contents = (id)laserImage.CGImage;
  leftbulletLayer.bounds = CGRectMake(0.0, 0.0, laserImage.size.width, laserImage.size.height);
  leftbulletLayer.anchorPoint = CGPointMake(1.0, 0.9);
  leftBullet.bulletLayer = leftbulletLayer;



  // Righ Bullet
  BulletObject* rightBullet = [BulletObject new];
  rightBullet.startPoint = CGPointMake(center.x + center.x, center.y - 160.0);
  rightBullet.endPoint = center;
  rightBullet.duration = 0.2;

  CALayer* rightBulletLayer = [CALayer layer];
  rightBulletLayer.backgroundColor = [UIColor clearColor].CGColor;
  rightBulletLayer.contents = (id)laserImage.CGImage;
  rightBulletLayer.bounds = CGRectMake(0.0, 0.0, laserImage.size.width, laserImage.size.height);
  rightBulletLayer.anchorPoint = CGPointMake(1.0, 0.9);
  rightBulletLayer.transform = CATransform3DMakeScale(-1.0, 1.0, 1.0);
  rightBullet.bulletLayer = rightBulletLayer;

  [self animateShotWithBullet:leftBullet];
  [self animateShotWithBullet:rightBullet];
}

- (void)animationDidStop:(CAAnimation *)anim finished:(BOOL)flag
{
  CALayer *layer = [anim valueForKey:@"animationLayer"];
  if(layer) {
    layer.opaque = NO;
    layer.opacity = 0.0;
    [layer removeFromSuperlayer];
    [layer removeAllAnimations];
  }
}

@end



@interface BulletObject ()
@property (assign, nonatomic) CGPoint currentPoint;
@end

@implementation BulletObject

- (void)setStartPoint:(CGPoint)startPoint
{
  _startPoint = startPoint;
}

- (void)update
{
//  NSTimeInterval timeDiff = [self.startTime timeIntervalSinceNow];
//  if (timeDiff > 0) {
//    cur
//  }
}

@end



