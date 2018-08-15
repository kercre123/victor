/**
* File: DeviceData
*
* Author: Lee Crippen
* Created: 06/16/16
*
* Description: Shared header for device data, implemented separately for each platform
*
* Copyright: Anki, Inc. 2016
*
**/

#ifndef __Anki_Cozmo_DeviceData_DeviceData_h__
#define __Anki_Cozmo_DeviceData_DeviceData_h__

#include "clad/types/deviceDataTypes.h"

#include <map>
#include <string>

namespace Anki {
namespace Vector {
  
using DeviceDataMap = std::map<DeviceDataType, std::string>;
  
class DeviceData
{
public:
  void Refresh();
  
  const DeviceDataMap& GetDataMap() const { return _dataMap; }
  
private:
  DeviceDataMap _dataMap;
};
  
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_DeviceData_DeviceData_h__
