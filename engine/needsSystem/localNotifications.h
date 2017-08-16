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

  void Init(const Json::Value& json, Util::RandomGenerator* rng);


  // Notify the OS to cancel all pending local notifications
  // (called when app opens or becomes un-backgrounded)
  void CancelAll();

  // Generate all appropriate local notifications, register them
  // with the OS (called when app is backgrounded)
  void Generate();

private:

  bool ShouldBeRegistered(const LocalNotificationItem& config) const;

  float DetermineTimeToNotify(const LocalNotificationItem& config,
                              const NeedsMultipliers& multipliers,
                              const float timeSinceAppOpen_m,
                              const Time now) const;
  float CalculateMinutesInFuture(const LocalNotificationItem& config,
                                 const float timeSinceAppOpen_m,
                                 const Time now) const;
  float CalculateMinutesToThreshold(const NeedId needId, const float targetLevel,
                                    const NeedsMultipliers& multipliers) const;

  NeedsManager& _needsManager;
  Util::RandomGenerator* _rng;

  LocalNotificationConfig _localNotificationConfig;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__

