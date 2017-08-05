/**
 * File: localNotifications
 *
 * Author: Paul Terry
 * Created: 08/02/2017
 *
 * Description: Local notifications for Cozmo's app
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/cozmo/basestation/needsSystem/localNotifications.h"
#include "anki/common/basestation/jsonTools.h"


namespace Anki {
namespace Cozmo {

static const std::string kLocalNotificationConfigsKey = "localNotificationConfigs";


LocalNotifications::LocalNotifications()
: _localNotificationConfig()
{
}

void LocalNotifications::Init(const Json::Value& json)
{
  _localNotificationConfig.items.clear();
  const auto& jsonLocalNotifications = json[kLocalNotificationConfigsKey];
  if (jsonLocalNotifications.isArray())
  {
    const s32 numEntries = jsonLocalNotifications.size();
    _localNotificationConfig.items.reserve(numEntries);

    for (int i = 0; i < numEntries; i++)
    {
      const Json::Value& jsonEntry = jsonLocalNotifications[i];
      LocalNotificationItem item;
      item.SetFromJSON(jsonEntry);
      _localNotificationConfig.items.emplace_back(item);
    }
  }
}


} // namespace Cozmo
} // namespace Anki

