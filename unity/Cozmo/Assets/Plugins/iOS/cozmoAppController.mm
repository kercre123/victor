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

#import "UnityAppController.h"
#include "anki/cozmo/csharp-binding/csharp-binding.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

// This App Controller is a subclass of the Unity generated AppController, registered with the #define at the bottom of this file
@interface CozmoAppController : UnityAppController
{
  UIBackgroundTaskIdentifier bgTask;
  bool _receivedWillResign;
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

bool checkInternetAvailable()
{
  NSURL* nsUrl = [[NSURL alloc] initWithString:@"https://www.google.com"];
  NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL:nsUrl];
  [request setAllowsCellularAccess:false];
  request.HTTPMethod = @"GET";

  __block NSError* error = nil;
  __block NSData* responseData = nil;

  dispatch_semaphore_t completionSignal = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    NSURLSessionTask* task = [[NSURLSession sharedSession] dataTaskWithRequest:request
                                completionHandler:^(NSData *data, NSURLResponse *blockResponse, NSError *blockError) {
                                  error = blockError;
                                  responseData = data;
                                  dispatch_semaphore_signal(completionSignal);
    }];
    [task resume];
  });
  dispatch_semaphore_wait(completionSignal, dispatch_time(DISPATCH_TIME_NOW, 10.0 * NSEC_PER_SEC));
  return (responseData && !error);
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

  // check for internet connectivity
  // the internet has all kinds of terrible looking advice for how to do this, I chose to refer to apple's documentation here:
  // https://developer.apple.com/library/ios/documentation/NetworkingInternetWeb/Conceptual/NetworkingOverview/WhyNetworkingIsHard/WhyNetworkingIsHard.html#//apple_ref/doc/uid/TP40010220-CH13-SW3
  if (checkInternetAvailable()) {
    NSLog(@"CozmoAppController.BackgroundFetch: executing transfers");
    cozmo_execute_background_transfers();
  }
  else {
    NSLog(@"CozmoAppController.BackgroundFetch: no internet");
  }
  
  // We always report that there's new data so that the OS will allow us to run as often as possible
  completionHandler(UIBackgroundFetchResultNewData);
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
  if (_receivedWillResign) {
    [super applicationWillResignActive:application];
    _receivedWillResign = false;
  }
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

- (void)applicationWillResignActive:(UIApplication*)application
{
  _receivedWillResign = true;
}

@end

IMPL_APP_CONTROLLER_SUBCLASS(CozmoAppController);
