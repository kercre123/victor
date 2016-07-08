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

#ifndef __DasPlatform_IOS_H__
#define __DasPlatform_IOS_H__

#include <DAS/DASPlatform.h>
#include <string>

namespace DAS {

class DASPlatform_IOS : public IDASPlatform
{
public:
  DASPlatform_IOS(std::string appRunId) : IDASPlatform(std::move(appRunId)) {}
  void Init();

  // app info
  virtual const char* GetAppVersion() const override { return _appVersion.c_str(); }

  // device/OS info
  virtual const char* GetDeviceId() const override { return _deviceId.c_str(); }
  virtual const char* GetDeviceModel() const override { return _deviceModel.c_str(); }
  virtual const char* GetOsVersion() const override { return _osVersion.c_str(); }
  virtual const char* GetCombinedSystemVersion() const override { return _combinedSystemVersion.c_str(); }
  virtual const char* GetPlatform() const override { return _platform.c_str(); }

  // storage/battery
  virtual const char* GetFreeDiskSpace() const override { return _freeDiskSpace.c_str(); }
  virtual const char* GetTotalDiskSpace() const override { return _totalDiskSpace.c_str(); }
  virtual const char* GetBatteryLevel() const override { return _batteryLevel.c_str(); }
  virtual const char* GetBatteryState() const override { return _batteryState.c_str(); }

  // other misc platform-specific info
  virtual const std::map<std::string, std::string>& GetMiscInfo() const override { return _miscInfo; }
  virtual const std::map<std::string, std::string>& GetMiscGlobals() const override { return _miscGlobals; }

private:
  std::string _appVersion;
  std::string _deviceId;
  std::string _deviceModel;
  std::string _osVersion;
  std::string _combinedSystemVersion;
  std::string _platform;
  std::string _freeDiskSpace;
  std::string _totalDiskSpace;
  std::string _batteryLevel;
  std::string _batteryState;
  std::map<std::string, std::string> _miscGlobals;
  std::map<std::string, std::string> _miscInfo;
};

}

#endif
