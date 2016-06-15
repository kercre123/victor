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
{
  UIBackgroundTaskIdentifier bgTask;
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;

- (void)application:(UIApplication *)application
performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;

@end

bool unityLogHandler(LogType logType, const char* log, va_list list)
{
    // Will log to stderr including system.log as a warning
    NSLogv([NSString stringWithUTF8String:log], list);
    return true;
}

@implementation CozmoAppController

- (id)init
{
  self = [super init];
  if (nil != self) {
    // [COZMO-2152] Always use NSLog for logging so everything in unity gets written to system.log
    // as a warning
    UnitySetLogEntryHandler(unityLogHandler);
  }
  return self;
}


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


- (void)applicationDidEnterBackground:(UIApplication*)application
{
  [super applicationDidEnterBackground:application];
  
  // Set up a task to throw onto the end of the main queue as a means of allowing whatevers in there a chance to complete
  if (self->bgTask == UIBackgroundTaskInvalid) {
    bgTask = [application beginBackgroundTaskWithExpirationHandler: ^{
      dispatch_async(dispatch_get_main_queue(), ^{
        [application endBackgroundTask:self->bgTask];
        self->bgTask = UIBackgroundTaskInvalid;
      });
    }];
  }
}

@end

IMPL_APP_CONTROLLER_SUBCLASS(CozmoAppController);
