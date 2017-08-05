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


#ifndef __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__
#define __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__

#include "anki/common/types.h"
#include "clad/types/needsSystemTypes.h"


namespace Anki {
namespace Cozmo {


class LocalNotifications
{
public:
  LocalNotifications();

  void Init(const Json::Value& json);

private:

  LocalNotificationConfig _localNotificationConfig;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__

