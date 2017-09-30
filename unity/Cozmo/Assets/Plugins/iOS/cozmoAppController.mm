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
#include "engine/cozmoAPI/csharp-binding/csharp-binding.h"
#include "util/ankiLab/extLabInterface.h"
#include "util/console/consoleInterface.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

namespace Anki {
  namespace Util {
    CONSOLE_VAR(bool, kDemoMode, "Demo", false);
  }
}

// This App Controller is a subclass of the Unity generated AppController, registered with the #define at the bottom of this file
@interface CozmoAppController : UnityAppController
{
  UIBackgroundTaskIdentifier bgTask;
  bool _receivedWillResign;
  UIActivityViewController* _currentActivityController;
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;

- (void)application:(UIApplication *)application
performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;

- (bool)exportCodelabFile:(NSString*)projectName withContent:(NSString*)content;

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

void tryExecuteBackgroundTransfers()
{
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
  
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
    [NSThread sleepForTimeInterval:2.0f];
    tryExecuteBackgroundTransfers();
  });
  
  REMOTE_CONSOLE_ENABLED_ONLY( Anki::Util::kDemoMode = false );
  
  BOOL didHandleURL = YES;
  
  NSURL *URL = [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey];
  if ( URL ) {
    didHandleURL = [self handleURL:URL];
  }
  
  return didHandleURL;
}

-(BOOL)handleOpenedFileURL:(NSURL *)filename {
  NSError *error;
  NSString *fullText = [NSString stringWithContentsOfURL:filename encoding:NSUTF8StringEncoding error:&error];

  if( fullText == nil ) {
    Unity_DAS_LogE("code_lab.ios.handle_opened_file_url.file_parse_error", [[error localizedDescription] UTF8String], nullptr, nullptr, 0);
  }
  else {
    NSUInteger codeLength = [fullText length];
    NSString* eventString = [@"size=" stringByAppendingString:[NSString stringWithFormat:@"%llu", (unsigned long long)codeLength]];
    Unity_DAS_Event("code_lab.ios.handle_opened_file_url", [eventString UTF8String], nullptr, nullptr, 0);

    UnitySendMessage("StartupManager", "LoadCodeLabFromRawJson", [fullText UTF8String]);
  }
  return YES;
}

- (BOOL)application:(UIApplication*)application openFile:(NSString *)filename
{
  return [self handleOpenedFileURL:[NSURL URLWithString:filename]];
}

// Handle URLs launched while app is running; copied from OD
- (BOOL)application:(UIApplication*)application openURL:(NSURL*)url sourceApplication:(NSString*)sourceApplication annotation:(id)annotation
{
  BOOL didHandleURL = YES;

  if ( url ) {
    didHandleURL = [self handleURL:url];
  }

  BOOL result = [super application:application openURL:url sourceApplication:sourceApplication annotation:annotation];
  didHandleURL = (didHandleURL & result);

  return didHandleURL;
}

- (BOOL)canHandleURLAsDemo:(NSURL *)URL {
  if ( [URL.scheme isEqualToString:@"cozmo"] ) {
    return YES;
  }
  return NO;
}
 
- (BOOL)canHandleURLAsCodelab:(NSURL *)URL {
  if ( [URL.scheme isEqualToString:@"file"] || [URL.scheme isEqualToString:@"content"] ) {
    return YES;
  }
  return NO;
}

- (BOOL)handleURL:(NSURL *)URL {
  // URLs should have the form:
  // cozmo://key/path?args
  // Example URL:
  // cozmo://settings/demo?enabled=true
  // Example Code:
  //  if ( [URL.host isEqualToString:@"settings"] ) {
  //    if ( [URL.path isEqualToString:@"/demo"] ) {
  //      if ( [URL.query isEqualToString:@"enabled=true"] ) {
  //        // do something...
  //      }
  //    }
  //  }
  //
  if ( [self canHandleURLAsDemo:URL]) {
    NSLog(@"Processing Launch URL");
    if ( [URL.host isEqualToString:@"settings"]) {
      if ( [URL.path isEqualToString:@"/demo"] ) {
        if ( [URL.query isEqualToString:@"enabled=true"] ) {
          REMOTE_CONSOLE_ENABLED_ONLY( Anki::Util::kDemoMode = true );
          return YES;
        }
      }
      else if ( [ URL.path isEqualToString:@"/abtesting"] ) {
        if ( [URL.query isEqualToString:@"enabled=true"] ) {
          Anki::Util::AnkiLab::EnableABTesting(true);
          return YES;
        } else if ( [URL.query isEqualToString:@"enabled=false"] ) {
          Anki::Util::AnkiLab::EnableABTesting(false);
          return YES;
        }
      } else if ( [ URL.path hasPrefix:@"/abtesting/force"] ) {
        Anki::Util::AnkiLab::HandleABTestingForceURL(std::string(URL.query.UTF8String));
        return YES;
      }
    }
  }
  
  if ( [self canHandleURLAsCodelab:URL] ) {
    [self handleOpenedFileURL:URL];
  }

  return NO;
}


// This handles the fetch when the OS says we should do one. Needs to call the passed in completionHandler with the appropriate result
-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler{
  
  tryExecuteBackgroundTransfers();
  
  // We always report that there's new data so that the OS will allow us to run as often as possible
  completionHandler(UIBackgroundFetchResultNewData);
}

- (void)presentActivityControllerForURL:(NSURL*)fileURL {

  if( _currentActivityController != nil )
  {
    [_currentActivityController dismissViewControllerAnimated:false completion:nil];
  }

  NSMutableArray *sharingItems = [NSMutableArray new];
  [sharingItems addObject:fileURL];

  UIActivityViewController* activityController = [[UIActivityViewController alloc] initWithActivityItems:sharingItems applicationActivities:nil];
  [activityController setExcludedActivityTypes:@[UIActivityTypeMail, UIActivityTypeAirDrop, UIActivityTypePrint, UIActivityTypeMessage, UIActivityTypeCopyToPasteboard, UIActivityTypeAssignToContact]];

  UIViewController *vc = self.window.rootViewController;
  activityController.popoverPresentationController.sourceView = vc.view;

  [activityController setCompletionWithItemsHandler:^(NSString *activityType, BOOL completed, NSArray *returnedItems, NSError *activityError) {
    _currentActivityController = nil;
  }];

  _currentActivityController = activityController;
  [vc presentViewController:activityController animated:YES completion:nil];
}

- (bool)exportCodelabFile:(NSString*)projectName withContent:(NSString*)content
{
  NSString *writablePath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)objectAtIndex:0];
  NSString *temporaryFilePath = [writablePath stringByAppendingPathComponent:@"cozmo-codelab"];

  NSFileManager* manager = [NSFileManager defaultManager];
  BOOL isDirectory;
  NSError *error = nil;
  if (![manager fileExistsAtPath:temporaryFilePath isDirectory:&isDirectory]) {

    NSDictionary *attr = [[NSDictionary alloc] init];

    [manager createDirectoryAtPath:temporaryFilePath
       withIntermediateDirectories:YES
                        attributes:attr
                             error:&error];
    if (error != nil) {
      NSString* errorString = [@"Error creating directory path: " stringByAppendingString:[error localizedDescription]];
      Unity_DAS_LogE("code_lab.ios.export_codelab_file.error_creating_temp_directory", [errorString UTF8String], nullptr, nullptr, 0);
      return false;
    }
  }
  else if (!isDirectory) {
    NSString* errorString = [@"Could not create path because a file is using the name wanted by our directory: " stringByAppendingString:[error localizedDescription]];
    Unity_DAS_LogE("code_lab.ios.export_codelab_file.temp_directory_name_in_use_by_file", [errorString UTF8String], nullptr, nullptr, 0);
    return false;
  }

  NSString *fileName = [projectName stringByAppendingString:@".codelab"];
  NSString *filePath = [temporaryFilePath stringByAppendingPathComponent:fileName];

  [content writeToFile:filePath
            atomically:YES
              encoding:NSUTF8StringEncoding
                 error:&error];

  if (error != nil) {
    NSString* errorString = [@"Codelab file export error: " stringByAppendingString:[error localizedDescription]];
    Unity_DAS_LogE("code_lab.ios.export_codelab_file.temp_file_create_error", [errorString UTF8String], nullptr, nullptr, 0);
  }
  else {
    NSURL *fileURL = [NSURL fileURLWithPath:filePath];
    [self presentActivityControllerForURL:fileURL];
    return true;
  }
  return false;
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

  // dismiss active activity controller if one exists
  if( _currentActivityController != nil )
  {
    [_currentActivityController dismissViewControllerAnimated:false completion:nil];
    _currentActivityController = nil;
  }
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
  Unity_DAS_Event("ios.memory_warning", "", nullptr, nullptr, 0);
  [super applicationDidReceiveMemoryWarning:application];
}

@end

extern "C"{
  // Method for Voice Command Settings
  void openAppSettings() {
    if (UIApplicationOpenSettingsURLString != NULL) {
      NSURL *appSettings = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
      [[UIApplication sharedApplication] openURL:appSettings];
    }
  }

  bool exportCodelabFile( const char* projectNameString, const char* projectContentString ) {

    NSString *formattedProjectName = [[[[NSString alloc] initWithCString:projectNameString encoding:NSUTF8StringEncoding] stringByReplacingOccurrencesOfString:@" " withString:@"_"] lowercaseString];

    NSString *content = [[NSString alloc] initWithCString:projectContentString encoding:NSUTF8StringEncoding];

    CozmoAppController* cozmoAppController = (CozmoAppController*)[[UIApplication sharedApplication] delegate];
    return [cozmoAppController exportCodelabFile:formattedProjectName withContent:content];
  }
}

IMPL_APP_CONTROLLER_SUBCLASS(CozmoAppController);
