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

#include "coretech/common/shared/types.h"
#include "engine/needsSystem/needsManager.h"
#include "clad/types/needsSystemTypes.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>


namespace Anki {
namespace Cozmo {

class CozmoContext;
class NeedsManager;

class LocalNotifications
{
public:
  LocalNotifications(const CozmoContext* context, NeedsManager& needsManager);
  ~LocalNotifications();

  void Init(const Json::Value& json, Util::RandomGenerator* rng,
            const float currentTime_s);

  void Update(const float currentTime_s);

  void SetPaused(const bool pausing);

  // Generate all appropriate local notifications, and register them
  // with the Game
  void Generate();
  
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

private:

  bool ShouldBeRegistered(const LocalNotificationItem& config) const;

  int DetermineTimeToNotify(const LocalNotificationItem& config,
                            const NeedsMultipliers& multipliers,
                            const float timeSinceAppOpen_m,
                            const Time now) const;
  float CalculateMinutesInFuture(const LocalNotificationItem& config,
                                 const float timeSinceAppOpen_m,
                                 const Time now) const;
  float CalculateMinutesToThreshold(const NeedId needId, const float targetLevel,
                                    const NeedsMultipliers& multipliers) const;

  const CozmoContext* _context;
  NeedsManager&           _needsManager;
  Util::RandomGenerator*  _rng;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  float                   _timeForPeriodicGenerate_s;

  LocalNotificationConfig _localNotificationConfig;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_LocalNotifications_H__

