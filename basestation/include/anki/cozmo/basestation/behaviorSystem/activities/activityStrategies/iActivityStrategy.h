/**
 * File: IActivityStrategy
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Interface for activity strategies (when activities become active/inactive)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_IActivityStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_IActivityStrategy_H__

#include "clad/types/featureGateTypes.h"

#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

template<typename TYPE> class AnkiEvent;
class MoodScorer;
class Robot;

class IActivityStrategy
{
public:

  // construction/destruction
  IActivityStrategy(const Json::Value& config);
  virtual ~IActivityStrategy();

  // true when this activity would be happy to start, false if it doens't want to be fired now
  bool WantsToStart(const Robot& robot,
                    float lastTimeActivityRanSec,
                    float lastTimeActivityStartedSec) const;

  // true when this activity wants to finish, false if it would rather continue
  bool WantsToEnd(const Robot& robot, float lastTimeActivityStartedSec) const;
  
  void SetCooldown(float cooldown_ms);
  
protected:

  // true when this activity would be happy to start, false if it doens't want to be fired now
  virtual bool WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const = 0;

  // true when this activity wants to finish, false if it would rather continue
  virtual bool WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const = 0;
  
  // allow access to ActivityShouldEnd in children to tell when the activity's about to be killed
  void SetActivityShouldEndSecs(float timeout_s){ _activityShouldEndSecs = timeout_s;}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // signal handles for events strategies register to
  std::vector<Signal::SmartHandle> _eventHandles;

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // activity runs for at least this much
  float _activityCanEndSecs;
  
  // activity runs for at most this much
  float _activityShouldEndSecs;

  // after finishing, the activity will not run again for this long plus a random value between 0 and
  // _cooldownRandomnessSecs
  float _baseCooldownSecs;
  
  // actual cooldown time calculated from base cooldown plus randomness
  // this is mutable because it gets calculated in WantsToStart when we are not in cooldown
  // TODO: Could add a OnSelected/OnDeselected to know when the activity this strategy belongs to
  // is actually started and stopped and calculate the next cooldown then
  mutable float _cooldownSecs;
  
  // a random value between 0 and this is added to _baseCooldownSecs to calculate _cooldownSecs
  float _cooldownVarianceSecs;
  
  // whether or not this strategy should start in cooldown
  // used to prevent the strategy from wanting to immediately start on startup
  bool _startInCooldown;
  
  // mood scoring to start the strategy. Only initialized if _requiredMinStartMoodScore is specified in json
  std::unique_ptr<MoodScorer> _startMoodScorer;
  
  // if _requiredMinStartMoodScore is set in json, the _startMoodScorer's score has to be greater than this value, for
  // the strategy to consider starting (other requirements may fail, so start is not guaranteed)
  float _requiredMinStartMoodScore;
  
  // if set, a recent OnTreads event is required, this being the number of seconds considered recent.
  // if the activity already started after the last event, it won't start unless a new event is fired
  float _requiredRecentOnTreadsEvent_secs;

  // optional feature gate to lock activity behind
  FeatureType _featureGate;
  
  void RandomizeCooldown(const Robot& robot) const;
};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_IActivityStrategy_H__
