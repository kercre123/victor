/**
 * File: platform/victorDAS/DASPlatform.h
 *
 * Description: Defines interface for platform-specific functionality
 *              needed to initialize DAS
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __victorDAS_DASPlatform_h
#define __victorDAS_DASPlatform_h

#include "DAS/DASPlatform.h"

namespace Anki {
namespace VictorDAS {

class DASPlatform : public DAS::IDASPlatform
{
public:
  DASPlatform(std::string appRunId) : IDASPlatform(std::move(appRunId)) {}

  // Getter methods
  virtual const char* GetAppVersion() const override { return _appVersion.c_str(); }
  virtual const char* GetDeviceId() const override { return _deviceId.c_str(); }
  virtual const char* GetDeviceModel() const override { return _deviceModel.c_str(); }
  virtual const char* GetOsVersion() const override { return _osVersion.c_str(); }
  virtual const char* GetCombinedSystemVersion() const override { return _combinedSystemVersion.c_str(); }
  virtual const char* GetPlatform() const override { return _platform.c_str(); }
  virtual const char* GetFreeDiskSpace() const override { return _freeDiskSpace.c_str(); }
  virtual const char* GetTotalDiskSpace() const override { return _totalDiskSpace.c_str(); }
  virtual const char* GetBatteryLevel() const override { return _batteryLevel.c_str(); }
  virtual const char* GetBatteryState() const override { return _batteryState.c_str(); }
  virtual const std::map<std::string, std::string>& GetMiscInfo() const override { return _miscInfo; }
  virtual const std::map<std::string, std::string>& GetMiscGlobals() const override { return _miscGlobals; }

  // Setter methods
  void SetAppVersion(const std::string & appVersion) { _appVersion = appVersion; }
  void SetDeviceId(const std::string & deviceId) { _deviceId = deviceId; }
  void SetDeviceModel(const std::string & deviceModel) { _deviceModel = deviceModel; }
  void SetOsVersion(const std::string & osVersion) { _osVersion = osVersion; }
  void SetCombinedSystemVersion(const std::string & comboVersion) { _combinedSystemVersion = comboVersion; }
  void SetPlatform(const std::string & platform) { _platform = platform; }
  void SetFreeDiskSpace(const std::string & freeDiskSpace) { _freeDiskSpace = freeDiskSpace; }
  void SetTotalDiskSpace(const std::string & totalDiskSpace) { _totalDiskSpace = totalDiskSpace; }
  void SetBatteryLevel(const std::string & batteryLevel) { _batteryLevel = batteryLevel; }
  void SetBatteryState(const std::string & batteryState) { _batteryState = batteryState; }
  void SetMiscInfo(const std::map<std::string, std::string> & miscInfo) { _miscInfo = miscInfo; }
  void SetMiscGlobals(const std::map<std::string, std::string> & miscGlobals) { _miscGlobals = miscGlobals; }

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

} // end namespace VictorDAS
} // end namespace Anki

#endif
