//
//  HockeyApp.m
//  cozmoGame
//
//  Created by Molly Jameson
//

#import <Foundation/Foundation.h>
#import "hockeyApp.h"

#import <DAS/DAS.h>
#import <DASClientInfo.h>
#import <HockeySDK/HockeySDK.h>



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
    DASEvent("HockeyApp.ios.handleOpenURL", "URL:%s sourceApp:%s", url.absoluteString.UTF8String, sourceApplication.UTF8String);
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
    DASEvent("HockeyApp.ios.opt_out", "");
    return;
  }

  NSString *hockeyAppId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.hockeyapp.appid"];
  if(!hockeyAppId || hockeyAppId.length == 0) {
    DASEvent("HockeyApp.ios.disabled", "");
    return;
  }

  DASEvent("HockeyApp.ios.checkin", "%s", hockeyAppId.UTF8String);

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
  NSMutableDictionary *metadata = [NSMutableDictionary dictionaryWithDictionary:@{ @"device": [DASClientInfo deviceID] }];
  // Check for cached DAS data in NSUserDefaults
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  
  NSString *lastAppRunID = [[DASClientInfo sharedInfo] lastAppRunID];
  if (!lastAppRunID) {
    // TODO (OD-2012): We need to eliminate or refactor DASClientInfo.  Here, we are directly
    // pulling the lastAppRunID from NSUserDefaults because DASClientInfo's eventsMainStart hasn't
    // been called yet.  This "fix" should be temporary.
    lastAppRunID = [defaults stringForKey:DASAppRunKey];
  }
  if ( [lastAppRunID isKindOfClass:[NSString class]] ) {
    metadata[@"apprun"] = lastAppRunID;
  }
  
  NSString *gameID = [defaults stringForKey:DASGameIDKey];
  if ( [gameID isKindOfClass:[NSString class]] ) {
    metadata[@"game"] = gameID;
  }
  
  NSString *userID = [defaults stringForKey:DASUserIDKey];
  if ( [userID isKindOfClass:[NSString class]] ) {
    metadata[@"name"] = userID;
  }
  
  NSError *error = nil;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:metadata options:NSJSONWritingPrettyPrinted error:&error];
  if ( !jsonData || error )
    return @"";
  
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
  /*
  NSString *hockeyAppId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.hockeyapp.appid"];
  if(!hockeyAppId || hockeyAppId.length == 0) {
    DASEvent("HockeyApp.ios.disabled", "");
    return;
  }
  [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:hockeyAppId];
  [[BITHockeyManager sharedHockeyManager] startManager];
  [[BITHockeyManager sharedHockeyManager].authenticator authenticateInstallation];
  */
  
  HockeyApp *hockeyApp = [[HockeyApp alloc] init];
  [hockeyApp activateHockeyApp];
}

