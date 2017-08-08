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
#include "engine/needsSystem/needsManager.h"
#include "clad/types/needsSystemTypes.h"


namespace Anki {
namespace Cozmo {

class NeedsManager;

class LocalNotifications
{
public:
  LocalNotifications(NeedsManager& needsManager);

  void Init(const Json::Value& json);


  // Notify the OS to cancel all pending local notifications
  // (called when app opens or becomes un-backgrounded)
  void CancelAll();

  // Generate all appropriate local notifications, register them
  // with the OS (called when app is backgrounded)
  void Generate();

private:

  bool ShouldBeRegistered(const LocalNotificationItem& config);

  float DetermineTimeToNotify(const LocalNotificationItem& config);
  float CalculateMinutesToThreshold(const NeedId needId, const float targetLevel);

  NeedsManager& _needsManager;
  
  LocalNotificationConfig _localNotificationConfig;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__

