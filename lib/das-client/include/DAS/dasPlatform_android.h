/**
 * File: dasPlatform_android
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

#ifndef __DasPlatform_Android_H__
#define __DasPlatform_Android_H__

#include <DAS/DASPlatform.h>
#include <jni.h>

namespace DAS {

class __attribute__((visibility("default"))) DASPlatform_Android : public IDASPlatform
{
public:
  DASPlatform_Android(std::string appRunId, std::string deviceUUIDPath) 
  : IDASPlatform(std::move(appRunId)) 
  , _deviceUUIDPath(std::move(deviceUUIDPath)) 
  {}

  // initialize with an instance of an android.content.Context Java object
  void Init(jobject appContext);
  // for Unity apps, initialize with the context provided by the Unity Player
  void InitForUnityPlayer();

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
  virtual const std::map<std::string, std::string>& GetMiscGlobals() const override;

  static void SetJVM(JavaVM* vm) { _jvm = vm; }

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
  std::string _deviceUUIDPath;
  std::map<std::string, std::string> _miscInfo;
  void Init(JNIEnv* env, jobject context);

  static JavaVM* _jvm;
};

}

#endif
