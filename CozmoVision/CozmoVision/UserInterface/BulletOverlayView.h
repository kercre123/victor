//
//  BulletOverlayView.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
@class BulletObject;

@interface BulletOverlayView : UIView

- (void)animateShotWithBullet:(BulletObject*)bullet;

@end


@interface BulletOverlayView (bulletFactory)

- (void)fireLaserButtlet;

@end



@interface BulletObject : NSObject
@property (assign, nonatomic) CGPoint startPoint;
@property (assign, nonatomic) CGPoint endPoint;
@property (assign, nonatomic) NSTimeInterval duration;
@property (assign, nonatomic) NSDate* startTime;
@property (strong, nonatomic) CALayer *bulletLayer;
@property (readonly) CGPoint currentPoint;

// Update Position
- (void)update;

@end