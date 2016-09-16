//
//  HockeyApp.m
//  cozmoGame
//
//  Created by Molly Jameson
//

#import <Foundation/Foundation.h>
#import "hockeyApp.h"

#import "util/logging/logging.h"
#import <DAS/DAS.h>
#import <DAS/DASPlatform.h>
#import <HockeySDK/HockeySDK.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif
  
  void UnitySendMessage(const char* obj, const char* method, const char* msg);
  
#ifdef __cplusplus
}
#endif


@interface HockeyApp : NSObject 

// Allow HockeyApp to handle URLs on launch
- (BOOL)handleOpenURL:(NSURL*)url
    sourceApplication:(NSString*)sourceApplication
           annotation:(id)annotation;

// Initialize HockeyApp crash reporting & connect to web service
- (void)activateHockeyApp;

- (NSString *)lastCrashDescriptionJSON;

@end


@interface HockeyApp () <BITHockeyManagerDelegate, BITCrashManagerDelegate>
@end

@implementation HockeyApp

#pragma mark - HockeyApp

BOOL gWaitingForCrashUpload = NO;

+ (void)waitForCrashUpload {
  int numWaits = 20; // 5 seconds.
  if(gWaitingForCrashUpload) {
    while(!gWaitingForCrashUpload && numWaits > 0) {
      NSLog(@"Waiting for HockeyApp Crash Upload... (%d/%d)", numWaits, 20);
      numWaits--;
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, NO); // sleep for a quarter second.
    }
  }
}

- (BOOL)handleOpenURL:(NSURL*)url
    sourceApplication:(NSString*)sourceApplication
           annotation:(id)annotation {
  BITAuthenticator* authenticator = [[BITHockeyManager sharedHockeyManager] authenticator];
  BOOL handled = [authenticator handleOpenURL:url
                            sourceApplication:sourceApplication   
                                   annotation:annotation];
  if (handled) {
    PRINT_NAMED_EVENT("HockeyApp.ios.handleOpenURL", "URL:%s sourceApp:%s", url.absoluteString.UTF8String, sourceApplication.UTF8String);
  }
  return handled;
}

- (BOOL)didCrashInLastSessionOnStartup {
  return ([[BITHockeyManager sharedHockeyManager].crashManager didCrashInLastSession] &&
          [[BITHockeyManager sharedHockeyManager].crashManager timeintervalCrashInLastSessionOccured] < 5);
}

- (void)activateHockeyApp {

  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  BOOL disableReporting = [defaults boolForKey:@"ReportingDisabled"];
  if (disableReporting) {
    PRINT_NAMED_EVENT("HockeyApp.ios.opt_out", "");
    return;
  }

  NSString *hockeyAppId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.hockeyapp.appid"];
  if(!hockeyAppId || hockeyAppId.length == 0) {
    PRINT_NAMED_EVENT("HockeyApp.ios.disabled", "");
    return;
  }
  PRINT_NAMED_INFO("HockeyApp.ios.checkin", "%s", hockeyAppId.UTF8String);

  [[BITHockeyManager sharedHockeyManager] configureWithBetaIdentifier:hockeyAppId
                                                       liveIdentifier:hockeyAppId
                                                             delegate:self];

#if !defined(ACTIVATE_HOCKEYAPP_UPDATENOTIFICATION)
  [BITHockeyManager sharedHockeyManager].disableUpdateManager = YES;
#endif
  [[BITHockeyManager sharedHockeyManager].authenticator setIdentificationType:BITAuthenticatorIdentificationTypeAnonymous];
  [[BITHockeyManager sharedHockeyManager] startManager];
  [[BITHockeyManager sharedHockeyManager].authenticator authenticateInstallation];

  // Startup crash detection
  if ([self didCrashInLastSessionOnStartup]) {
    gWaitingForCrashUpload = YES;
    // we wait for this flag to clear in the AppLoader.
  } else {
    // do normal initialization
    [self setupApplication];
  }
  
  
  
  // Logging unity exceptions, UnitySendMessage function to unity it only supports one string param.
  std::string comma_string(hockeyAppId.UTF8String);
  NSString *versionCode = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
  comma_string += ",";
  if( versionCode )
  {
    comma_string += [versionCode UTF8String];
  }
  NSString *versionName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
  comma_string += ",";
  if( versionName )
  {
    comma_string += [versionName UTF8String];
  }
  NSString *bundleIdentifier = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleIdentifier"];
  comma_string += ",";
  if( bundleIdentifier )
  {
    comma_string += [bundleIdentifier UTF8String];
  }
  comma_string += ",";
  const DAS::IDASPlatform* platform = DASGetPlatform();
  // DAS data like apprun
  if( platform )
  {
    comma_string += platform->GetDeviceId();
  }
  comma_string += ",";
  comma_string += ",";
  NSString* sdkVersion =  [[BITHockeyManager sharedHockeyManager] version];
  if( sdkVersion )
  {
    comma_string += [sdkVersion UTF8String];
  }
  
  // SDK Name... that is apparently not defined in the SDK according to the example source
  comma_string += ",HockeySDK";
  UnitySendMessage("StartupManager", "UploadUnityCrashInfoIOS", comma_string.c_str());
  
}

-(void)setupApplication {
  // let us get past waiting for crash upload state.
  gWaitingForCrashUpload = NO;
}

#pragma mark - HockeyApp Delegates

// BITUpdateManagerDelegate
- (NSString *)customDeviceIdentifierForUpdateManager:(BITUpdateManager *)updateManager {
#ifndef CONFIGURATION_AppStore
  return [[UIDevice currentDevice] identifierForVendor].UUIDString;
#endif
  return nil;
}

- (NSString *)lastCrashDescriptionJSON {
  const DAS::IDASPlatform* dasPlatform = DASGetPlatform();
  if (dasPlatform == nullptr) {
    assert(false);
    return @"";
  }
  NSString *deviceId = [NSString stringWithUTF8String:dasPlatform->GetDeviceId()];
  NSMutableDictionary *metadata = [NSMutableDictionary dictionaryWithDictionary:@{ @"device": deviceId }];

  const auto& miscInfoMap = dasPlatform->GetMiscInfo();
  const auto lastAppRunIt = miscInfoMap.find("lastAppRunId");
  if (lastAppRunIt != miscInfoMap.end()) {
    NSString *appRunString = [NSString stringWithUTF8String:lastAppRunIt->second.c_str()];
    metadata[@"apprun"] = appRunString;
  }
  
  NSError *error = nil;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:metadata options:NSJSONWritingPrettyPrinted error:&error];
  if ( !jsonData || error ) {
    return @"";
  }
  
  NSString *jsonMetadata = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  return jsonMetadata;
}


#pragma mark - BITCrashManagerDelegate
-(NSString *)applicationLogForCrashManager:(BITCrashManager *)crashManager {
  return [self lastCrashDescriptionJSON];
}

- (void)crashManagerWillCancelSendingCrashReport:(BITCrashManager *)crashManager {
  if ([self didCrashInLastSessionOnStartup]) {
    [self setupApplication];
  }
}

- (void)crashManager:(BITCrashManager *)crashManager didFailWithError:(NSError *)error {
  if ([self didCrashInLastSessionOnStartup]) {
    [self setupApplication];
  }
}

- (void)crashManagerDidFinishSendingCrashReport:(BITCrashManager *)crashManager {
  if ([self didCrashInLastSessionOnStartup]) {
    [self setupApplication];
  }
}

@end


void CreateHockeyApp()
{
  // Example simple
//  NSString *hockeyAppId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.hockeyapp.appid"];
//  if(!hockeyAppId || hockeyAppId.length == 0) {
//    DASEvent("HockeyApp.ios.disabled", "");
//    return;
//  }
//  [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:hockeyAppId];
//  [[BITHockeyManager sharedHockeyManager] startManager];
//  [[BITHockeyManager sharedHockeyManager].authenticator authenticateInstallation];
  
  HockeyApp *hockeyApp = [[HockeyApp alloc] init];
  [hockeyApp activateHockeyApp];
}

