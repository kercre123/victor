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

#ifndef __Anki_Cozmo_DeviceData_DeviceDataManager_h__
#define __Anki_Cozmo_DeviceData_DeviceDataManager_h__

#include "coretech/common/engine/utils/signals/simpleSignal_fwd.h"

#include <map>
#include <memory>
#include <list>

namespace Anki {
namespace Vector {
  
class DeviceData;
class IExternalInterface;
  
class DeviceDataManager
{
public:
  DeviceDataManager(IExternalInterface* externalInterface);
  virtual ~DeviceDataManager();
  
  // For handling various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  std::unique_ptr<DeviceData>       _deviceData;
  IExternalInterface*               _externalInterface = nullptr;
  std::list<::Signal::SmartHandle>  _signalHandles;
};
  
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_DeviceData_DeviceDataManager_h__
