//
//  AppDelegate.h
//  CozmoVision
//
//  Created by Andrew Stein on 11/25/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
@class CozmoEngineWrapper;
@class CozmoOperator;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) CozmoEngineWrapper* cozmoEngineWrapper;
@property (strong, nonatomic) CozmoOperator*      cozmoOperator;


@end

