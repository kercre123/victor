/**
 * File: cozmoAppController.mm
 *
 * Author: Lee Crippen
 * Created: 12/13/15
 *
 * Description: Handles Objective-C events for background data transfer.
 * WARNING: THIS FILE IS COPIED OVER FROM THE SOURCE FOUND IN THE UNITY ASSETS, SO MODIFY IT THERE.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "UnityAppController.h"
#import "DAS/DAS.h"


// This App Controller is a subclass of the Unity generated AppController, registered with the #define at the bottom of this file
@interface CozmoAppController : UnityAppController

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;

- (void)application:(UIApplication *)application
performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;

@end

@implementation CozmoAppController

// This adds the launch option to say we want the BackgroundFetch to actually happen sometimes (whenever the OS decrees)
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
  [super application:application didFinishLaunchingWithOptions:launchOptions];
  
  [application setMinimumBackgroundFetchInterval:UIApplicationBackgroundFetchIntervalMinimum];
  
  return YES;
}

// This handles the fetch when the OS says we should do one. Needs to call the passed in completionHandler with the appropriate result
-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler{
  DASForceFlushNow();
  
  // We always report that there's new data so that the OS will allow us to run as often as possible
  completionHandler(UIBackgroundFetchResultNewData);
}

@end

IMPL_APP_CONTROLLER_SUBCLASS(CozmoAppController);
