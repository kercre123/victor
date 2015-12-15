#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "UnityAppController.h"

// This BackgroundTransfer not currently implemented at all, merely here as placeholder
@interface BackgroundTransfer : NSObject <NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLSessionDownloadDelegate>

@end

// This App Controller is a subclass of the Unity generated AppController, registered with the #define at the bottom
@interface MyAppController : UnityAppController

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;

- (void)application:(UIApplication *)application
performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;

@end

@implementation MyAppController

// This adds the launch option to say we want the BackgroundFetch to actually happen sometimes (whenever the OS decrees)
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
  [super application:application didFinishLaunchingWithOptions:launchOptions];
  
  [application setMinimumBackgroundFetchInterval:UIApplicationBackgroundFetchIntervalMinimum];
  
  return YES;
}

// This handles the fetch when the OS says we should do one. Needs to call the passed in completionHandler with the appropriate result
-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler{
  
  NSURLSessionConfiguration *sessionConfiguration = [NSURLSessionConfiguration defaultSessionConfiguration];
  NSURLSession *session = [NSURLSession sessionWithConfiguration:sessionConfiguration];
  
  NSURL *url = [[NSURL alloc] initWithString:@"http://www.anki.com"];
  NSURLSessionDataTask *task = [session dataTaskWithURL:url
                                      completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                                        if (error) {
                                          completionHandler(UIBackgroundFetchResultFailed);
                                          return;
                                        }
                                        
                                        //NSData *dataCast = data;
                                        NSString *dataString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                                        
                                        ::printf("%s", [dataString UTF8String]);
                                        
                                        completionHandler(UIBackgroundFetchResultNewData);
                                      }];
  // Start the task
  [task resume];
}

@end

IMPL_APP_CONTROLLER_SUBCLASS(MyAppController);
