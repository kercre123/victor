/**
 * File: dasPlatform_ios
 *
 * Author: baustin
 * Created: 07/05/16
 *
 * Description: Defines interface for platform-specific functionality
 *              needed to initialize DAS
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "dasPlatform_ios.h"
#include <sstream>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <sys/sysctl.h>

namespace DAS {

static std::string sysctlValueByName(const char* key)
{
  std::string ret;
  size_t bufferSize = 0;
  sysctlbyname(key, nullptr, &bufferSize, nullptr, 0);

  if (bufferSize == 0) {
    return ret;
  }

  char buffer[bufferSize];
  bzero(buffer, sizeof(char) * bufferSize);
  int result = sysctlbyname(key, buffer, &bufferSize, nullptr, 0);

  if (result >= 0) {
    ret = buffer;
  }
  return ret;
}

static std::string sysctlValueForKey(const int key)
{
  std::string ret;
  int mib[2] = {CTL_KERN, key};
  u_int namelen = sizeof(mib) / sizeof(mib[0]);
  size_t bufferSize = 0;

  // Get the size for the buffer
  sysctl(mib, namelen, nullptr, &bufferSize, nullptr, 0);

  if (bufferSize == 0) {
    return ret;
  }

  char buildBuffer[bufferSize];
  bzero(buildBuffer, sizeof(char) * bufferSize);
  int result = sysctl(mib, namelen, buildBuffer, &bufferSize, nullptr, 0);

  if (result >= 0) {
    ret = buildBuffer;
  }
  return ret;
}

void DASPlatform_IOS::Init()
{
  // app version
  NSString *dasVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.das.version"];
  _appVersion = dasVersion.UTF8String;

  // device id
  if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)]) {
    _deviceId = [[[UIDevice currentDevice] identifierForVendor] UUIDString].UTF8String;
  }
  else {
    _deviceId = "0";
  }

  // device model
  _deviceModel = sysctlValueByName("hw.machine");

  // combined system version
  NSString *systemName = [[UIDevice currentDevice] systemName];
  NSString *iosVersion = [[UIDevice currentDevice] systemVersion];
  std::string osBuildVersion = sysctlValueForKey(KERN_OSVERSION);
  {
    NSString *s = [NSString stringWithFormat:@"%@-%s-%@-%s",
                   systemName,
                   _deviceModel.c_str(),
                   iosVersion,
                   osBuildVersion.c_str()];
    _combinedSystemVersion = s.UTF8String;
  }

  // build OS version string
  {
    NSString *s = [NSString stringWithFormat:@"%@-%@-%s",
                   systemName,
                   iosVersion,
                   osBuildVersion.c_str()];
    _osVersion = s.UTF8String;
  }

  // platform
  _platform = "ios";

  // free disk space
  NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
  {
    NSString *s = [NSString stringWithFormat:@"%@", [fattributes objectForKey:NSFileSystemFreeSize]];
    _freeDiskSpace = s.UTF8String;
  }

  // total disk space
  {
    NSString *s = [NSString stringWithFormat:@"%@", [fattributes objectForKey:NSFileSystemSize]];
    _totalDiskSpace = s.UTF8String;
  }

  // battery level
  [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
  _batteryLevel = [NSString stringWithFormat:@"%1.2f", [[UIDevice currentDevice] batteryLevel]].UTF8String;

  // battery state
  {
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
    _batteryState = status.UTF8String;
  }

  // set tag, if exists
  NSString *dastag = [[NSUserDefaults standardUserDefaults] stringForKey:@"das_tag"];
  if (dastag) {
    _miscGlobals.emplace(std::string{"$tag"}, std::string{dastag.UTF8String});
  }

  // last apprun
  NSString *lastAppRunKey = @"lastAppRunId";
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSString *lastAppRunId = [defaults stringForKey:lastAppRunKey];
  if (lastAppRunId) {
    _miscInfo.emplace(std::string{lastAppRunKey.UTF8String}, std::string{lastAppRunId.UTF8String});
  }
  NSString* newAppRunId = [NSString stringWithCString:GetAppRunId() encoding:[NSString defaultCStringEncoding]];
  [defaults setObject:newAppRunId forKey:lastAppRunKey];
  
  // Background refresh status
  const std::string kBackgroundRefreshStatusKey = "device.background_refresh_status";
  UIBackgroundRefreshStatus status = [[UIApplication sharedApplication] backgroundRefreshStatus];
  switch (status) {
    case UIBackgroundRefreshStatusAvailable:
      // We can do background fetch! Let's do this!
      _miscInfo.emplace(kBackgroundRefreshStatusKey, "available");
      break;
    case UIBackgroundRefreshStatusDenied:
      // The user has background fetch turned off. Too bad.
      _miscInfo.emplace(kBackgroundRefreshStatusKey, "denied");
      break;
    case UIBackgroundRefreshStatusRestricted:
      // Parental Controls, Enterprise Restrictions, Old Phones, Oh my!
      _miscInfo.emplace(kBackgroundRefreshStatusKey, "restricted");
      break;
    default:
      _miscInfo.emplace(kBackgroundRefreshStatusKey, std::to_string(status));
      break;
  }
}

}
