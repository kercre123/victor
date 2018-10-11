/**
 * File: BehaviorGreetAfterLongTime.cpp
 *
 * Author: Hamzah Khan
 * Created: 2018-06-25
 *
 * Description: This behavior causes victor to, upon seeing a face, react "loudly" if he hasn't seen the person for a certain amount of time.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorGreetAfterLongTime.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/faceWorld.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/DAS.h"

#include "clad/types/variableSnapshotIds.h"
#include "clad/types/animationTrigger.h"

#include <iostream>
#include <iomanip>
#include <ctime>

namespace Anki {
namespace Vector {
  
namespace {

// faceWorld constants
const bool kRecognizableFacesOnly = true;
const float kMaxTimerAfterSeenToReact_s = 5.0f;

#if REMOTE_CONSOLE_ENABLED
static BehaviorGreetAfterLongTime::LastSeenMapPtr sMapPtr;

void ForceNextBigGreeting(ConsoleFunctionContextRef context)
{
  if( sMapPtr ) {
    BehaviorGreetAfterLongTime::DebugPrintState("beforeForceGreeting", sMapPtr);

    // set all stored timestamps to a really old time
    for( auto& namePair : *sMapPtr ) {
      namePair.second = time_t(0); // should set to epoch (in any case, a long time ago)
    }

    BehaviorGreetAfterLongTime::DebugPrintState("afterForceGreeting", sMapPtr);
  }
}
#endif

}

#define CONSOLE_GROUP "BehaviorBigGreeting"

CONSOLE_FUNC(ForceNextBigGreeting, CONSOLE_GROUP);
CONSOLE_VAR(bool, kBigGreetingDriveOffCharger, CONSOLE_GROUP, true);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::DynamicVariables::DynamicVariables()
  : lastSeenTimesPtr(std::make_shared<std::unordered_map<std::string, time_t>>()),
    lastFaceCheckTime_ms(0),
    shouldActivateBehaviorUntil_s(-1.0f),
    timeSinceSeenThisFace_s(0),
    thisCooldown_s(0)
{
#if REMOTE_CONSOLE_ENABLED
  if( sMapPtr == nullptr ) {
    sMapPtr = lastSeenTimesPtr;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::BehaviorGreetAfterLongTime(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::~BehaviorGreetAfterLongTime()
{
#if REMOTE_CONSOLE_ENABLED
  if( sMapPtr == _dVars.lastSeenTimesPtr ) {
    sMapPtr = nullptr;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID( BEHAVIOR_ID( DriveOffChargerFace ) );

  auto& variableSnapshotComp = GetBEI().GetVariableSnapshotComponent();
  variableSnapshotComp.InitVariable<std::unordered_map<std::string, time_t>>(VariableSnapshotId::BehaviorGreetAfterLongTime_LastSeenTimes,
                                                                        _dVars.lastSeenTimesPtr);
  
  DebugPrintState("init");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.driveOffChargerBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::DebugPrintState(const char* debugLabel) const
{
  DebugPrintState(debugLabel, _dVars.lastSeenTimesPtr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::DebugPrintState(const char* debugLabel, const LastSeenMapPtr mapPtr)
{
  PRINT_CH_INFO("Behaviors", "BehaviorGreetAfterLongTime.DebugState",
                "%s", debugLabel);

  if( !mapPtr ) {
    PRINT_NAMED_WARNING("BehaviorGreetAfterLongTime.DebugState.NoPointer", "pointer is null");
    return;
  }
  
  for( const auto& namePair : *mapPtr ) {
    std::stringstream timeSS;
    std::tm tm = *std::localtime(&namePair.second);
    timeSS << std::put_time(&tm, "%F %T");
    
    PRINT_CH_INFO("Behaviors", "BehaviorGreetAfterLongTime.DebugState.Entry",
                  "%s, %s",
                  Util::HidePersonallyIdentifiableInfo(namePair.first.c_str()),
                  timeSS.str().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGreetAfterLongTime::WantsToBeActivatedBehavior() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool shouldActivate = _dVars.shouldActivateBehaviorUntil_s > 0.0f &&
                              _dVars.shouldActivateBehaviorUntil_s <= currTime_s;
  return shouldActivate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::OnBehaviorActivated() 
{
  DebugPrintState("OnBehaviorActivated");

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const int timeSince_ms = std::round( (currTime_s - _dVars.shouldActivateBehaviorUntil_s) * 1000.0f );
  
  DASMSG(big_greeting_played, "behavior.big_greeting.played",
         "We saw a face that we haven't seen in a while, playing a big greeting");
  DASMSG_SET(i1, _dVars.timeSinceSeenThisFace_s, "seconds since we last saw this face");
  DASMSG_SET(i2, _dVars.thisCooldown_s, "what the cooldown was for this instance of the big greeting");
  DASMSG_SET(i3, timeSince_ms, "milliseconds since we saw the face in BaseStationTime");
  DASMSG_SEND();

  // TODO:(bn) use normal pattern here of resetting all non-persistent variables
  _dVars.shouldActivateBehaviorUntil_s = -1.0f; // don't activate again (right away)

  if( GetBEI().GetRobotInfo().IsOnChargerPlatform() &&
      kBigGreetingDriveOffCharger &&
      _iConfig.driveOffChargerBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.driveOffChargerBehavior.get(), &BehaviorGreetAfterLongTime::TurnTowardsFace );
  }
  else {
    TurnTowardsFace();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::BehaviorUpdate() 
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( !IsActivated() &&
      _dVars.shouldActivateBehaviorUntil_s > 0.0f &&
      _dVars.shouldActivateBehaviorUntil_s < currTime_s ) {
    // we missed the boat on this reaction
    _dVars.shouldActivateBehaviorUntil_s = -1.0f;

    DASMSG(big_greeting_missed, "behavior.big_greeting.missed",
           "We saw a face that we haven't seen in a while, but failed to play the big greeting in time because "
           "we were doing other things");
    DASMSG_SET(i1, _dVars.timeSinceSeenThisFace_s, "seconds since we last saw this face");
    DASMSG_SET(i2, _dVars.thisCooldown_s, "what the cooldown was for this instance of the big greeting");
    DASMSG_SEND();
  }
  
  // get the walltime as a time_t
  time_t unixUtcTime_s;
  if( !ShouldBehaviorUpdate(unixUtcTime_s) ) {
    return;
  }

  const int kDontCheckRelativeAngles = 0;
  const int kDontCheckRangeOfAngles = 0;

  const auto& faceWorld = GetBEI().GetFaceWorld();

  // get recognized faces seen since last face update
  auto faceIds = faceWorld.GetFaceIDs(_dVars.lastFaceCheckTime_ms, 
                                      kRecognizableFacesOnly,
                                      kDontCheckRelativeAngles,
                                      kDontCheckRangeOfAngles);

  for(const auto& faceId : faceIds) {
    const auto* face = faceWorld.GetFace(faceId);
    std::string name = face->GetName();

    // greeting should only happen if the face is recognized and named
    if(!name.empty()) {
      // if face has not been seen for long enough, react strongly
      auto faceLastSeenTimeIter = _dVars.lastSeenTimesPtr->find(name);

      // if the face is newly recognizable, add it to the map with a greeting cooldown starting now
      const bool isFaceNewlySeen = faceLastSeenTimeIter == _dVars.lastSeenTimesPtr->end();
      if(isFaceNewlySeen) {
        _dVars.lastSeenTimesPtr->emplace(name, unixUtcTime_s);
      }
      else {
        time_t faceLastSeenTime_s = faceLastSeenTimeIter->second;

        // flag behavior for activation if we see the face after long enough. It can run immediately or in a
        // few seconds, but not after too long
        const auto cooldownPeriod_s = GetCooldownPeriod_s();
        const bool shouldActivate = (faceLastSeenTime_s + cooldownPeriod_s <= unixUtcTime_s);
        if( shouldActivate ) {
          _dVars.shouldActivateBehaviorUntil_s = currTime_s + kMaxTimerAfterSeenToReact_s;
          _dVars.targetFace = faceWorld.GetSmartFaceID(faceId);

          // save some variables now for DAS later
          _dVars.timeSinceSeenThisFace_s = (int)( (long int)unixUtcTime_s - (long int)faceLastSeenTime_s );
          _dVars.thisCooldown_s = cooldownPeriod_s;

          DebugPrintState("shouldActivate");
        }
 
        // regardless of a reaction, reset the cooldown
       _dVars.lastSeenTimesPtr->at(name) = unixUtcTime_s;
      }

    // keeps track of the last time a face was seen to avoid rechecking old faces
    // necessary because vision may add faces into the past after completing processing
    // note: times stored in lastFaceCheckTime_ms use robot time and are not compatible with WallTime
    _dVars.lastFaceCheckTime_ms = std::max(_dVars.lastFaceCheckTime_ms, (RobotTimeStamp_t)face->GetTimeStamp());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGreetAfterLongTime::ShouldBehaviorUpdate(time_t& unixUtcTime_s)
{
  std::tm utcTimeObj;
  auto* wt = WallTime::getInstance();
  // TODO:(bn) switch back to non-approximate time once we trust that it is working...
  const bool gotTime = wt->GetApproximateUTCTime(utcTimeObj);
  unixUtcTime_s = std::mktime(&utcTimeObj);

  // only proceed if behavior is not activated and walltime is properly synced
  return !IsActivated() && gotTime;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::TurnTowardsFace()
{

  if( _dVars.targetFace.IsValid() ) {
    DelegateIfInControl( new TurnTowardsFaceAction( _dVars.targetFace ), [this]() {
        // don't need the target face anymore
        _dVars.targetFace.Reset();
        PlayReactionAnimation();
      });
  }
  else {
    PRINT_NAMED_WARNING("BehaviorGreetAfterLongTime.TurnTowardsFace.NoFace",
                        "Face that triggered behavior is invalid, skipping turn");
    PlayReactionAnimation();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::PlayReactionAnimation()
{
  
  // value for number of loops argument for playing anim once
  const int kPlayAnimOnce = 1;
  // value for whether starting the animation can interrupt
  const bool kCanAnimationInterrupt = true;


  TriggerAnimationAction* action = new TriggerAnimationAction(AnimationTrigger::GreetAfterLongTime,
                                                              kPlayAnimOnce,
                                                              kCanAnimationInterrupt);
  DelegateNow(action);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint BehaviorGreetAfterLongTime::GetCooldownPeriod_s() const
{
  const float alive_h = GetBehaviorComp<RobotStatsTracker>().GetNumHoursAlive();

  if( alive_h < 24.0f ) {
    return 30 * 60;
  }
  else if( alive_h < 168.0f ) { // 1 week
    return 60 * 60;
  }
  else if( alive_h < 336.0f ) { // 2 weeks
    return 2 * 60 * 60;
  }

  return 4 * 60 * 60;
}

}
}
