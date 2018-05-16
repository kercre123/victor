/**
 * File: behaviorDanceToTheBeat.h
 *
 * Author: Matt Michini
 * Created: 2018-05-07
 *
 * Description: Dance to a beat detected by the microphones/beat detection algorithm
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/behaviorDanceToTheBeat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "engine/components/bodyLightComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/random/randomGenerator.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

namespace {
  const char* kDanceDuration_key = "danceDuration_sec";
  
  const char* kDanceDurationVariability_key = "danceDurationVariability_sec";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::BehaviorDanceToTheBeat(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
: danceDuration_sec(JsonTools::ParseFloat(config, kDanceDuration_key, debugName))
, danceDurationVariability_sec(JsonTools::ParseFloat(config, kDanceDurationVariability_key, debugName))
{
  DEV_ASSERT(danceDuration_sec > danceDurationVariability_sec,
             "BehaviorDanceToTheBeat.InstanceConfig.InvalidParameters");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kDanceDuration_key,
    kDanceDurationVariability_key
  };
  expectedKeys.insert(std::begin(list), std::end(list));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::WantsToBeActivatedBehavior() const
{
  // TODO: For now, this behavior never wants to be activated, since
  //       we don't have dancing animations yet.
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorActivated()
{
  // Reset dynamic variables:
  _dVars = DynamicVariables();
  
  // grab the next beat time here and wait to play the anim until the appropriate time
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  _dVars.nextAnimTime_sec = beatDetector.GetNextBeatTime_sec();
  _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  const float danceDurationVariability_sec = GetRNG().RandDblInRange(-_iConfig.danceDurationVariability_sec,
                                                                      _iConfig.danceDurationVariability_sec);
  _dVars.dancingEndTime_sec = now_sec + _iConfig.danceDuration_sec + danceDurationVariability_sec;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  if (now_sec > _dVars.dancingEndTime_sec) {
    if (!IsControlDelegated()) {
      if (_dVars.getoutAnimPlayed) {
        CancelSelf();
      } else {
        DelegateIfInControl(new PlayAnimationAction("anim_petting_bliss_getout_01"));
        _dVars.getoutAnimPlayed = true;
      }
    }
  } else if(now_sec > _dVars.nextAnimTime_sec) {
    // Time to play an animation. For now, just pulse the backpack lights on each beat
    static const BackpackLights lights = {
      .onColors               = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{60,60,60}},
      .offPeriod_ms           = {{1000,1000,1000}},
      .transitionOnPeriod_ms  = {{0,0,0}},
      .transitionOffPeriod_ms = {{200,200,200}},
      .offset                 = {{0,0,0}}
    };

    GetBEI().GetBodyLightComponent().SetBackpackLights(lights);
    
    _dVars.nextAnimTime_sec += _dVars.beatPeriod_sec;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorDeactivated()
{
  // Reset the beat detector so that we don't immediately go back into this behavior
  GetBEI().GetBeatDetectorComponent().Reset();
}


}
}

