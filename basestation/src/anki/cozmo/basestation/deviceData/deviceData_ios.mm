/**
* File: deviceData (ios impl)
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
#include "TargetConditionals.h"

#if TARGET_OS_IPHONE
#import <DASClientInfo.h>
#endif // TARGET_OS_IPHONE

namespace Anki {
namespace Cozmo {
  
#if TARGET_OS_IPHONE
void DeviceData::Refresh()
{
  _dataMap.clear();
  
  // A couple pieces of the data are universal and not tied to the dasclientinfo instance
  _dataMap[DeviceDataType::DeviceID] = [DASClientInfo deviceID].UTF8String;
  _dataMap[DeviceDataType::DeviceModel] = [DASClientInfo model].UTF8String;
  
  
  // For everything else we need toget the das client info instance and pull the data off it
  DASClientInfo* dasSharedInfo = [DASClientInfo sharedInfo];
  if (nil == dasSharedInfo)
  {
    ASSERT_NAMED(nil == dasSharedInfo, "DeviceData.Refresh.DASClientInfo.InstanceMissing");
    return;
  }
  
  NSString* tempString = nil;
  
  // Grab the AppRunID if it exists
  tempString = [dasSharedInfo appRunID];
  if (nil != tempString)
  {
    _dataMap[DeviceDataType::AppRunID] = tempString.UTF8String;
  }
  
  // Grab the LastAppRunID if it exists
  tempString = [dasSharedInfo lastAppRunID];
  if (nil != tempString)
  {
    _dataMap[DeviceDataType::LastAppRunID] = tempString.UTF8String;
  }
  
  // Grab the GameID if it exists
  tempString = [dasSharedInfo gameID];
  if (nil != tempString)
  {
    _dataMap[DeviceDataType::GameID] = tempString.UTF8String;
  }
  
  // Grab the UserID if it exists
  tempString = [dasSharedInfo userID];
  if (nil != tempString)
  {
    _dataMap[DeviceDataType::UserID] = tempString.UTF8String;
  }
  
  tempString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.das.version"];
  if (nil != tempString)
  {
    _dataMap[DeviceDataType::BuildVersion] = tempString.UTF8String;
  }
}
#else // TARGET_OS_IPHONE
void DeviceData::Refresh() { }
#endif

} // namespace Cozmo
} // namespace Anki
