/**
* File: DeviceDataManager
*
* Author: Lee Crippen
* Created: 06/16/16
*
* Description: Manages requests for device data
*
* Copyright: Anki, Inc. 2016
*
**/

#include "engine/deviceData/deviceDataManager.h"

#include "engine/ankiEventUtil.h"
#include "engine/deviceData/deviceData.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
DeviceDataManager::DeviceDataManager(IExternalInterface* externalInterface)
: _deviceData(new DeviceData)
, _externalInterface(externalInterface)
{
  if (nullptr == _externalInterface)
  {
    return;
  }
  
  auto helper = MakeAnkiEventUtil(*externalInterface, *this, _signalHandles);
  helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::RequestDeviceData>();
}
  
DeviceDataManager::~DeviceDataManager() = default;

template<>
void DeviceDataManager::HandleMessage(const ExternalInterface::RequestDeviceData& msg)
{
  DEV_ASSERT(nullptr != _externalInterface, "DeviceDataManager.HandleMessage.NullExternalInterface");
  
  _deviceData->Refresh();
  const auto& dataMap = _deviceData->GetDataMap();
  
  std::vector<DeviceDataPair> dataPairs;
  for (const auto& pair : dataMap)
  {
    dataPairs.emplace_back(pair.first, pair.second);
  }
  
  _externalInterface->BroadcastToGame<ExternalInterface::DeviceDataMessage>(std::move(dataPairs));
}
  
  
} // namespace Vector
} // namespace Anki
