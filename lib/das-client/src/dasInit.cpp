/**
 * File: dasInit
 *
 * Author: baustin
 * Created: 07/05/16
 *
 * Description: Utilizes the provided IDASPlatform to set up DAS
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>

static std::unique_ptr<const DAS::IDASPlatform> sPlatform;

void DASNativeInit(std::unique_ptr<const DAS::IDASPlatform> platform, const char* product)
{
  if (sPlatform != nullptr) {
    DASError("DASNativeInit", "static platform is already set!");
  }
  DAS_SetGlobal("$app", platform->GetAppVersion());
  DAS_SetGlobal("$apprun", platform->GetAppRunId());
  DAS_SetGlobal("$phone", platform->GetCombinedSystemVersion());
  DAS_SetGlobal("$unit", platform->GetDeviceId());
  DAS_SetGlobal("$platform", platform->GetPlatform());

  DAS_SetGlobal("$messv", "2");
  DAS_SetGlobal("$product", product);

  for (const auto& kvp : platform->GetMiscGlobals()) {
    DAS_SetGlobal(kvp.first.c_str(), kvp.second.c_str());
  }

  DASEvent("app.version", "%s", platform->GetAppVersion());
  DASEvent("app.launch", "%s", platform->GetAppRunId());
  DASEvent("device.model", "%s", platform->GetDeviceModel());
  DASEvent("device.os_version", "%s", platform->GetOsVersion());
  DASEvent("device.free_disk_space", "%s", platform->GetFreeDiskSpace());
  DASEvent("device.total_disk_space", "%s", platform->GetTotalDiskSpace());
  DASEvent("device.battery_level", "%s", platform->GetBatteryLevel());
  DASEvent("device.battery_state", "%s", platform->GetBatteryState());

  for (const auto& kvp : platform->GetMiscInfo()) {
    DASEvent(kvp.first.c_str(), "%s", kvp.second.c_str());
  }

  sPlatform = std::move(platform);
}

const DAS::IDASPlatform* DASGetPlatform()
{
  return sPlatform.get();
}
