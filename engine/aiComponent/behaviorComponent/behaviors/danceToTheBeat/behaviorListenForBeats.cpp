/**
 * File: BehaviorListenForBeats.cpp
 *
 * Author: Matt Michini
 * Created: 2018-08-20
 *
 * Description: Listens for beats from the BeatDetectorComponent, playing animations along the way
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/behaviorListenForBeats.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/components/mics/beatDetectorComponent.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kPreListeningAnim_key  = "preListeningAnim";
  const char* kListeningAnim_key     = "listeningAnim";
  const char* kPostListeningAnim_key = "postListeningAnim";
  const char* kMinListeningTime_key  = "minListeningTime_sec";
  const char* kMaxListeningTime_key  = "maxListeningTime_sec";
  const char* kCancelSelfIfBeatLost_key = "cancelSelfIfBeatLost";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorListenForBeats::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  preListeningAnim   = AnimationTrigger::Count;
  listeningAnim      = AnimationTrigger::Count;
  postListeningAnim  = AnimationTrigger::Count;
  
  JsonTools::GetCladEnumFromJSON(config, kPreListeningAnim_key,  preListeningAnim,  debugName);
  JsonTools::GetCladEnumFromJSON(config, kListeningAnim_key,     listeningAnim,     debugName);
  JsonTools::GetCladEnumFromJSON(config, kPostListeningAnim_key, postListeningAnim, debugName);
  
  minListeningTime_sec = JsonTools::ParseFloat(config, kMinListeningTime_key, debugName);
  maxListeningTime_sec = JsonTools::ParseFloat(config, kMaxListeningTime_key, debugName);
  
  cancelSelfIfBeatLost = JsonTools::ParseBool(config, kCancelSelfIfBeatLost_key, debugName);
  
  DEV_ASSERT(minListeningTime_sec < maxListeningTime_sec, "BehaviorListenForBeats.InstanceConfig.InvalidMinMaxTimes");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorListenForBeats::BehaviorListenForBeats(const Json::Value& config)
 : ICozmoBehavior(config)
  , _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorListenForBeats::~BehaviorListenForBeats()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorListenForBeats::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPreListeningAnim_key,
    kListeningAnim_key,
    kPostListeningAnim_key,
    kMinListeningTime_key,
    kMaxListeningTime_key,
    kCancelSelfIfBeatLost_key
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // Play the pre-listening animation then transition to listening
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.preListeningAnim),
                      &BehaviorListenForBeats::TransitionToListening);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::BehaviorUpdate() 
{
  const bool isListening = _dVars.listeningStartTime_sec > 0.f;
  if (!IsActivated() || !isListening) {
    return;
  }

  // Since we are now listening for beats, reset the listen for beats cooldown timer
  GetBEI().GetBehaviorTimerManager().GetTimer(BehaviorTimerTypes::ListenForBeatsCooldown).Reset();
  
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float listeningTime_sec = now_sec - _dVars.listeningStartTime_sec;
  
  // Do nothing until we've listened for the minimum length of time
  if (listeningTime_sec < _iConfig.minListeningTime_sec) {
    return;
  }
  
  bool shouldEnd = false;
  
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (listeningTime_sec > _iConfig.maxListeningTime_sec) {
    PRINT_NAMED_WARNING("BehaviorListenForBeats.BehaviorUpdate.Timeout",
                        "No beat detected after maximum listening period of %.2f seconds",
                        _iConfig.maxListeningTime_sec);
    shouldEnd = true;
  } else if (beatDetector.IsBeatDetected()) {
    // A legit beat is detected - play the post listening anim and exit
    shouldEnd = true;
  } else if (!beatDetector.IsPossibleBeatDetected() && _iConfig.cancelSelfIfBeatLost) {
    PRINT_NAMED_WARNING("BehaviorListenForBeats.BehaviorUpdate.NoMoreBeat",
                        "Cancelling since beat detector says there is no possible beat detected");
    shouldEnd = true;
  }
  
  if (shouldEnd) {
    DelegateNow(new TriggerAnimationAction(_iConfig.postListeningAnim));
    _dVars.listeningStartTime_sec = -1.f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorListenForBeats::TransitionToListening()
{
  _dVars.listeningStartTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  DelegateIfInControl(new ReselectingLoopAnimationAction(_iConfig.listeningAnim, 0)); // loop forever
}
  

}
}
