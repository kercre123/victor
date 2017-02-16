//
//  DASClientInfo.m
//  DAS
//
//  Created by tom on 1/3/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#import "DASClientInfo.h"
#import <UIKit/UIKit.h>
#import "DAS.h"
#import <sys/sysctl.h>

NSString * const DASAppRunKey = @"DASAppRunID";
NSString * const DASGameIDKey = @"DASGameID";
NSString * const DASUserEmailKey = @"DASUserEmailKey";
NSString * const DASUserIDKey = @"DASUserIDKey";

NSString* DASPeriodicNotification = @"DASPeriodicNotifiation";
DASClientInfo* gDASClientInfo = nil;

@interface DASClientInfo ()
@property (nonatomic, readwrite, weak) NSTimer *dasPeriodicTimer;
@property (nonatomic, readwrite, copy) NSString *appRunID;
@property (nonatomic, readwrite, copy) NSString *lastAppRunID;
@end

@implementation DASClientInfo

@synthesize dasPeriodicTimer = _dasPeriodicTimer;
@synthesize userID = _userID;
@synthesize gameID = _gameID;
@synthesize appRunID = _appRunID;
@synthesize lastAppRunID = _lastAppRunID;

+ (DASClientInfo *)sharedInfo {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    gDASClientInfo = [[DASClientInfo alloc] init];
  });
  return gDASClientInfo;
}

// This file is separated into two sections
// The first is methods named events<Stage> where stage
// is some stage of the program such as main start, etc.
// This section is used to set client info in batch.
//
// The second section are the utility methods used to obtain
// client info such as unique IDs, software versions, etc.
//
//

// EVENTS SECTION
- (void) eventsMainStart:(const char *)dasUser appVersion:(const char *)appVersion {
    
  [self eventsMainStart:dasUser appVersion:appVersion product:"od"];
}

- (void) eventsMainStart:(const char *)dasUser appVersion:(const char *)appVersion product:(const char *)product {
  
  // SETGlobal for app start
  DAS_SetGlobal("$app", appVersion);

  self.appRunID = [DASClientInfo appRunID];
  DAS_SetGlobal("$apprun", gDASClientInfo.appRunID.UTF8String);
  DAS_SetGlobal("$messv", "2");
  DAS_SetGlobal("$phone", [DASClientInfo combinedSystemVersion].UTF8String);
  DAS_SetGlobal("$unit", [DASClientInfo deviceID].UTF8String);
  DAS_SetGlobal("$product", product);
  DAS_SetGlobal("$platform", "ios");
  
  const char *dastag = [DASClientInfo dasTag].UTF8String;
  if (dastag) {
      DAS_SetGlobal("$tag", dastag);
  }
  
  // SEND App Launch message
  DASEvent("app.version", "%s", appVersion);
  DASEvent("app.launch", "%s", gDASClientInfo.appRunID.UTF8String);
  //DASEvent("Device.Name", "%s", [DASClientInfo deviceName].UTF8String);
  DASEvent("device.model", "%s", [DASClientInfo model].UTF8String);
  DASEvent("device.os_version", "%s-%s-%s", [DASClientInfo systemName].UTF8String, [DASClientInfo iosVersion].UTF8String, [DASClientInfo osBuildVersion].UTF8String);
  DASEvent("device.free_disk_space", "%s", [DASClientInfo freeDiskSpace].UTF8String);
  DASEvent("device.total_disk_space", "%s", [DASClientInfo totalDiskSpace].UTF8String);
  DASEvent("device.battery_level", "%s", [DASClientInfo batteryLevel].UTF8String);
  DASEvent("device.battery_state", "%s", [DASClientInfo batteryState].UTF8String);

  // DAS Events are not logged to the console.
  // Dump App Info to console to provide info for DAS queries.
  NSLog(@"ApplicationVersion - %s", appVersion);
  NSLog(@"ApplicationLaunched - %s", gDASClientInfo.appRunID.UTF8String);
  
  /* setup the DAS periodic event timer.  Clients register for the DASPeriodicNotification and post periodic events at this interval */
  NSTimer *periodicTimer = [NSTimer timerWithTimeInterval:30.0 target:gDASClientInfo selector:@selector(dasTimerFired:) userInfo:nil repeats:YES];
  [[NSRunLoop mainRunLoop] addTimer:periodicTimer forMode:NSDefaultRunLoopMode];
  self.dasPeriodicTimer = periodicTimer;
  
  // listen for App events
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];
  
} // end eventsMainStart

// TIMER CALLBACK (for periodic DAS event pulse)
-(void)dasTimerFired:(NSTimer*)timer {
  [[NSNotificationCenter defaultCenter] postNotificationName:DASPeriodicNotification object:nil];
}

// CLIENT INFO UTILITY METHODS
+ (NSString *) appRunID {
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    NSString *uuidAppRun = (__bridge_transfer NSString*)CFUUIDCreateString(kCFAllocatorDefault, uuid);
    CFRelease(uuid);
    return uuidAppRun;
}

+ (NSString *) dasTag {
    NSString *dastag = [[NSUserDefaults standardUserDefaults] stringForKey:@"das_tag"];
    if (dastag) {
        return dastag;
    }
    return NULL;
}

// This will be unique for every device, and the same for all our apps on that device.
// This will be reset once a user deletes the final Anki (vendor) app from their device.
// This will also be reset on some major OS updates, potentially.
+ (NSString *) deviceID {
    if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)]) {
        return [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    }
    return @"0";
}

// last 12 digits of vendorIdentifier
+ (NSString *)supportVendorID {
  NSString *UUIDString = [self deviceID];
  // 12 characters + first '-'
  NSString *shortID = [UUIDString substringToIndex:13];
  return shortID;
}

+ (NSString *) combinedSystemVersion {
  NSString *s = [NSString stringWithFormat:@"%@-%@-%@-%@",
                 [DASClientInfo systemName],
                 [DASClientInfo model],
                 [DASClientInfo iosVersion],
                 [DASClientInfo osBuildVersion]];
  return s;
}

// 6.0
+ (NSString *)iosVersion {
    return [[UIDevice currentDevice] systemVersion];
}

+ (NSString *)osBuildVersion {
  return [self sysctlValueForKey:KERN_OSVERSION];
}

// iPhone3,1, iPod2,1
+ (NSString *) model {
  NSString *machine = [self sysctlValueByName:"hw.machine"];
  return machine;
}

+ (NSString *)sysctlValueByName:(const char *)key {
  size_t bufferSize = 0;
  sysctlbyname(key, NULL, &bufferSize, NULL, 0);
  
  if ( bufferSize == 0 )
    return nil;
  
  unsigned char buffer[bufferSize];
  bzero(buffer, sizeof(unsigned char) * bufferSize);
  int result = sysctlbyname(key, buffer, &bufferSize, NULL, 0);

  NSString *v = nil;
  if (result >= 0) {
    v = [[NSString alloc] initWithUTF8String:(const char *)buffer];
  }
  
  return v;
}

+ (NSString *)sysctlValueForKey:(const int)key {
  int mib[2] = {CTL_KERN, (int)key};
  u_int namelen = sizeof(mib) / sizeof(mib[0]);
  size_t bufferSize = 0;
  
  // Get the size for the buffer
  sysctl(mib, namelen, NULL, &bufferSize, NULL, 0);
  
  if (bufferSize == 0)
    return nil;
  
  unsigned char buildBuffer[bufferSize];
  bzero(buildBuffer, sizeof(unsigned char) * bufferSize);
  int result = sysctl(mib, namelen, buildBuffer, &bufferSize, NULL, 0);
  
  NSString *v = nil;
  if (result >= 0) {
    v = [[NSString alloc] initWithUTF8String:(const char *)buildBuffer];
  }
  
  return v;
}

// System Name (not sure what this is other than iOS)
+ (NSString *) systemName {
    return [[UIDevice currentDevice] systemName];
}

+ (NSString *) freeDiskSpace {
    NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
    NSString *s = [NSString stringWithFormat:@"%@", [fattributes objectForKey:NSFileSystemFreeSize]];
    return s;
}

+ (NSString *) totalDiskSpace {
    NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
    NSString *s = [NSString stringWithFormat:@"%@", [fattributes objectForKey:NSFileSystemSize]];
    return s;
}

// my iphone
+ (NSString *) deviceName {
    return [[UIDevice currentDevice] name];
}

+ (NSString *) batteryLevel {
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    
    [[UIDevice currentDevice] batteryLevel];
    return [NSString stringWithFormat:@"%1.2f", [[UIDevice currentDevice] batteryLevel]];
}

+ (NSString *) batteryState {
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    NSString *status = @"unknown";
    
    NSArray *batteryStatus = [NSArray arrayWithObjects:
                              @"unknown.",
                              @"discharging",
                              @"charging",
                              @"charged", nil];
    
    if ([[UIDevice currentDevice] batteryState] != UIDeviceBatteryStateUnknown) {
        status = [NSString stringWithFormat:
                  @"%@",
                  [batteryStatus objectAtIndex:[[UIDevice currentDevice] batteryState]] ];
    }
    return status;
} // end batteryState

#pragma mark - Notification Events

- (void)applicationWillTerminate:(NSNotification *)notification {
  [self clearCachedProperties];
}

#pragma mark - properties

- (void)clearCachedProperties {
  [[NSUserDefaults standardUserDefaults] removeObjectForKey:DASGameIDKey];
  [[NSUserDefaults standardUserDefaults] removeObjectForKey:DASUserEmailKey];
}

- (void)setAppRunID:(NSString *)appRunID {
  _appRunID = appRunID;
  
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  self.lastAppRunID = [defaults stringForKey:DASAppRunKey];
  if ( _appRunID ) {
    [defaults setObject:_appRunID forKey:DASAppRunKey];
  } else {
    [defaults removeObjectForKey:DASAppRunKey];
  }
}

- (void)setGameID:(NSString *)gameID {
  _gameID = gameID;

  // store to NSUserDefaults in case we crash
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  if ( [_gameID isKindOfClass:[NSString class]] ) {
    [defaults setObject:_gameID forKey:DASGameIDKey];
    DAS_SetGlobal("$game", _gameID.UTF8String);
  } else {
    [defaults removeObjectForKey:DASGameIDKey];
    DAS_SetGlobal("$game", NULL);
  }
}

- (void)setUserID:(NSString *)userID {
  _userID = userID;
  
  // store to NSUserDefaults in case we crash
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  if ( [_userID isKindOfClass:[NSString class]] ) {
    [defaults setObject:_userID forKey:DASUserIDKey];
    DAS_SetGlobal("$user", _userID.UTF8String);
  } else {
    [defaults removeObjectForKey:DASUserIDKey];
    DAS_SetGlobal("$user", NULL);
  }
}

@end
