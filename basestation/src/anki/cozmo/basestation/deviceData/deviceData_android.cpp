/**
* File: deviceData (android impl)
*
* Author: Lee Crippen
* Created: 06/16/16
*
* Description: Shared header for device data, implemented separately for each platform
*
* Copyright: Anki, Inc. 2016
*
**/

#include "anki/cozmo/basestation/deviceData/deviceData.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
void DeviceData::Refresh()
{
  _dataMap.clear();
  
  //TODO:(lc) This is where we need to fill out the map of device data for the android platform
  // A couple pieces of the data are universal and not tied to the dasclientinfo instance
  PRINT_NAMED_ERROR("DeviceData.Refresh.AndroidImplMissing", "");
}

} // namespace Cozmo
} // namespace Anki
