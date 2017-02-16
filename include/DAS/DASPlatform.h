/**
 * File: DASPlatform
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

#ifndef __DasPlatform_H__
#define __DasPlatform_H__

#include <map>
#include <string>

namespace DAS {

class IDASPlatform
{
public:
  // app must specify this
  IDASPlatform(std::string appRunId) : _appRunId(std::move(appRunId)) {}
  virtual ~IDASPlatform() = default;

  // app info
  virtual const char* GetAppVersion() const = 0;
  virtual const char* GetAppRunId() const { return _appRunId.c_str(); }

  // device/OS info
  virtual const char* GetDeviceId() const = 0;
  virtual const char* GetDeviceModel() const = 0;
  virtual const char* GetOsVersion() const = 0;
  virtual const char* GetCombinedSystemVersion() const = 0;
  virtual const char* GetPlatform() const = 0;

  // storage/battery
  virtual const char* GetFreeDiskSpace() const = 0;
  virtual const char* GetTotalDiskSpace() const = 0;
  virtual const char* GetBatteryLevel() const = 0;
  virtual const char* GetBatteryState() const = 0;

  // other misc platform-specific info
  virtual const std::map<std::string, std::string>& GetMiscInfo() const = 0;
  virtual const std::map<std::string, std::string>& GetMiscGlobals() const = 0;

private:
  IDASPlatform(const IDASPlatform&) = delete;
  IDASPlatform& operator=(const IDASPlatform&) = delete;
  std::string _appRunId;
};

}

#endif
