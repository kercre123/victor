//
//  ShootingCozmoViewController.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ShootingCozmoViewController : UIViewController

@property(assign, nonatomic) float   maxTargetingSoundPeriod_sec;
@property(assign, nonatomic) BOOL    useAudioTargeting;
@property(assign, nonatomic) BOOL    useVisualTargeting;

@end
