/**
* File: deviceData
*
* Author: Lee Crippen
* Created: 06/16/16
*
* Description: Shared header for device data, implemented separately for each platform
*
* Copyright: Anki, Inc. 2016
*
**/

#include "engine/deviceData/deviceData.h"
#include "util/logging/logging.h"

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

namespace Anki {
namespace Vector {

#if USE_DAS
void DeviceData::Refresh()
{
  _dataMap.clear();

  const DAS::IDASPlatform* platform = DASGetPlatform();
  if (nullptr == platform)
  {
    DEV_ASSERT(false, "DASPlatform.InstanceMissing");
    return;
  }

  _dataMap[DeviceDataType::DeviceID] = platform->GetDeviceId();
  _dataMap[DeviceDataType::DeviceModel] = platform->GetDeviceModel();
  _dataMap[DeviceDataType::AppRunID] = platform->GetAppRunId();
  _dataMap[DeviceDataType::BuildVersion] = platform->GetAppVersion();

  // Grab the LastAppRunID if it exists
  const auto& miscInfoMap = platform->GetMiscInfo();
  const auto it = miscInfoMap.find("lastAppRunId");
  if (it != miscInfoMap.end())
  {
    _dataMap[DeviceDataType::LastAppRunID] = it->second;
  }
}
#else
void DeviceData::Refresh() {}
#endif

} // namespace Vector
} // namespace Anki
