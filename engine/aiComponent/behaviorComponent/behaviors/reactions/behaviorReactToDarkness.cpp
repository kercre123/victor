/**
 * File: behaviorReactToDarkness.cpp
 *
 * Author: Humphrey Hu
 * Created: 2018-06-03
 *
 * Description: Behavior to react when the lights go out
 * 
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToDarkness.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIlluminationDetected.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"

#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

#include <iterator>

namespace Anki {
namespace Cozmo {

namespace {
static const char* kLookHeadAngleMinKey   = "lookHeadAngleMin_deg";
static const char* kLookHeadAngleMaxKey   = "lookHeadAngleMax_deg";
static const char* kLookWaitTimeKey       = "lookWaitTime_s";
static const char* kLookTurnAngleMinKey   = "lookTurnAngleMin_deg";
static const char* kLookTurnAngleMaxKey   = "lookTurnAngleMax_deg";
static const char* kLookMinImages         = "lookMinImages";
static const char* kNumOffChargerLooksKey = "numOffChargerLooks";
static const char* kNumOnChargerLooksKey  = "numOnChargerLooks";
static const char* kEmotionKey            = "reactionEmotion";
static const char* kSleepInPlaceKey       = "sleepInPlace";
}

BehaviorReactToDarkness::BehaviorReactToDarkness( const Json::Value& config )
: ICozmoBehavior( config )
{
  _iConfig.lookHeadAngleMin_deg = JsonTools::ParseFloat( config, kLookHeadAngleMinKey,
                                                         "BehaviorReactToDarkness.Constructor" );
  _iConfig.lookHeadAngleMax_deg = JsonTools::ParseFloat( config, kLookHeadAngleMaxKey,
                                                         "BehaviorReactToDarkness.Constructor" );
  _iConfig.lookWaitTime_s = JsonTools::ParseFloat( config, kLookWaitTimeKey,
                                                   "BehaviorReactToDarkness.Constructor" );
  _iConfig.lookTurnAngleMax_deg = JsonTools::ParseFloat( config, kLookTurnAngleMaxKey,
                                                         "BehaviorReactToDarkness.Constructor" );
  _iConfig.lookTurnAngleMin_deg = JsonTools::ParseFloat( config, kLookTurnAngleMinKey,
                                                         "BehaviorReactToDarkness.Constructor" );
  _iConfig.numOnChargerLooks = JsonTools::ParseUInt32( config, kNumOnChargerLooksKey,
                                                       "BehaviorReactToDarkness.Constructor" );
  _iConfig.numOffChargerLooks = JsonTools::ParseUInt32( config, kNumOffChargerLooksKey,
                                                       "BehaviorReactToDarkness.Constructor" );
  _iConfig.sleepInPlace = JsonTools::ParseBool( config, kSleepInPlaceKey,
                                                "BehaviorReactToDarkness.Constructor" );
  _iConfig.reactionEmotion = JsonTools::ParseString( config, kEmotionKey,
                                                     "BehaviorReactToDarkness.Constructor" );
  
  const u32 lookMinImages = JsonTools::ParseUInt32( config, kLookMinImages,
                                                    "BehaviorReactToDarkness.Constructor" );
  // NOTE These are hard-coded so they don't have to be in the config JSON
  _iConfig.positiveConfig = IBEICondition::GenerateBaseConditionConfig( BEIConditionType::IlluminationDetected );
  _iConfig.positiveConfig["triggerStates"].append(EnumToString(IlluminationState::Darkened));
  _iConfig.positiveConfig["confirmationTime"] = _iConfig.lookWaitTime_s;
  _iConfig.positiveConfig["confirmationMinNum"] = lookMinImages;
  _iConfig.positiveConfig["ignoreUnknown"] = false;

  _iConfig.negativeConfig = IBEICondition::GenerateBaseConditionConfig( BEIConditionType::IlluminationDetected );
  _iConfig.negativeConfig["triggerStates"].append(EnumToString(IlluminationState::Illuminated));
  _iConfig.negativeConfig["confirmationTime"] = _iConfig.lookWaitTime_s;
  _iConfig.negativeConfig["confirmationMinNum"] = lookMinImages;
  _iConfig.negativeConfig["ignoreUnknown"] = false;

  _iConfig.homeFilter = std::make_unique<BlockWorldFilter>();
}

bool BehaviorReactToDarkness::WantsToBeActivatedBehavior() const
{
  return _iConfig.positiveCondition->AreConditionsMet( GetBEI() );
}

void BehaviorReactToDarkness::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
  const char* list[] = {
    kLookHeadAngleMinKey,
    kLookHeadAngleMaxKey,
    kLookTurnAngleMinKey,
    kLookTurnAngleMaxKey,
    kLookWaitTimeKey,
    kLookMinImages,
    kNumOffChargerLooksKey,
    kNumOnChargerLooksKey,
    kEmotionKey,
    kSleepInPlaceKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

void BehaviorReactToDarkness::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( !_iConfig.sleepInPlace )
  {
    delegates.insert( _iConfig.goHomeBehavior.get() );
  }
}

void BehaviorReactToDarkness::InitBehavior()
{
  _iConfig.positiveCondition.reset( new ConditionIlluminationDetected( _iConfig.positiveConfig ) );
  _iConfig.positiveCondition->SetOwnerDebugLabel( GetDebugLabel() );
  _iConfig.positiveCondition->Init( GetBEI() );
  _iConfig.negativeCondition.reset( new ConditionIlluminationDetected( _iConfig.negativeConfig ) );
  _iConfig.negativeCondition->SetOwnerDebugLabel( GetDebugLabel() );
  _iConfig.negativeCondition->Init( GetBEI() );
  _dVars.Reset();

  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.goHomeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(FindAndGoToHome));
  DEV_ASSERT( _iConfig.goHomeBehavior != nullptr,
              "BehaviorReactToDarkness.InitBehavior.NullGoHomeBehavior" );
}

void BehaviorReactToDarkness::OnBehaviorActivated()
{
  _iConfig.positiveCondition->Reset();
  _iConfig.negativeCondition->Reset();
  _dVars.Reset();
}

void BehaviorReactToDarkness::OnBehaviorEnteredActivatableScope()
{
  _iConfig.positiveCondition->SetActive( GetBEI(), true );
  _iConfig.negativeCondition->SetActive( GetBEI(), true );
}

void BehaviorReactToDarkness::OnBehaviorLeftActivatableScope()
{
  _iConfig.negativeCondition->SetActive( GetBEI(), false );
  _iConfig.positiveCondition->SetActive( GetBEI(), false );
}

void BehaviorReactToDarkness::BehaviorUpdate()
{
  if( !IsActivated() )
  { 
    return;
  }

  switch( _dVars.state )
  {
    case State::Waiting:
    {
      TransitionToTurning();
      break;
    }
    case State::Turning:
    {
      break;
    }
    case State::TurnedAndWaiting:
    {
      f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( Util::IsFltGT( currTime, _dVars.waitStartTime + _iConfig.lookWaitTime_s ) )
      {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.BehaviorUpdate.Timeout", "");
        CancelSelf();
      }

      // Illumination check succeeded, increment counter
      if( _iConfig.positiveCondition->AreConditionsMet( GetBEI() ) )
      {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.BehaviorUpdate.PositiveMet", "" );
        _dVars.numChecksSucceeded++;
        _dVars.waitStartTime = currTime;
      }
      // Illumination check failed
      else if( _iConfig.negativeCondition->AreConditionsMet( GetBEI() ) )
      {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.BehaviorUpdate.NegativeMet", "" );
        CancelSelf();
      }
      // Not sure yet, keep waiting
      else
      {
        break;
      }

      if( LookedEnoughTimes() )
      {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.BehaviorUpdate.RunningTriggers", "" );
        TransitionToResponding();
      }
      else
      {
        TransitionToTurning();
      }
      break;
    }
    case State::Responding:
    {
      // Waiting for animation to finish
      break;
    }
    case State::GoingHome:
    {
      if( !IsControlDelegated() )
      {
        CancelSelf();
      }
      break;
    }
  }
}

void BehaviorReactToDarkness::DynamicVariables::Reset()
{
  state = State::Waiting;
  numChecksSucceeded = 0;
}

void BehaviorReactToDarkness::TransitionToTurning()
{
  _dVars.state = State::Turning;

  // Sample a new head angle angle
  Util::RandomGenerator& rng = GetRNG();
  const double headAngle_deg = rng.RandDblInRange( _iConfig.lookHeadAngleMin_deg, _iConfig.lookHeadAngleMax_deg );
  
  // Create a compound action, to which we'll add a turn if appropriate
  CompoundActionParallel* action = new CompoundActionParallel({
    new MoveHeadToAngleAction( DEG_TO_RAD( headAngle_deg ) )
  });
  
  // NOTE If we're on platform, don't turn in place, but still go home if triggering
  if( !GetBEI().GetRobotInfo().IsOnChargerPlatform() )
  {
    // If not on the charger, also rotate the body
    const double turnSign = (rng.RandInt(2) == 0 ? -1.0 : 1.0);
    const double turnMagnitude = rng.RandDblInRange( _iConfig.lookTurnAngleMin_deg, _iConfig.lookTurnAngleMax_deg );
    const double turnAngle_deg = turnSign * turnMagnitude;
    PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.TransitionToTurning.SetHeadAndTurn",
                  "Setting head to %.1fdeg and turning %f", headAngle_deg, turnAngle_deg );

    action->AddAction( new TurnInPlaceAction( DEG_TO_RAD( turnAngle_deg ), false ) );
  }
  else
  {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.TransitionToTurning.SetHead",
                  "Setting head to %.1fdeg and not turning because on charger", headAngle_deg );
  }
  
  DelegateIfInControl( action, &BehaviorReactToDarkness::TransitionToTurnedAndWaiting );
}

void BehaviorReactToDarkness::TransitionToTurnedAndWaiting()
{
  _dVars.state = State::TurnedAndWaiting;
  _dVars.waitStartTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

bool BehaviorReactToDarkness::LookedEnoughTimes() const
{
  const u32 numLooks = GetBEI().GetRobotInfo().IsOnChargerContacts() ? _iConfig.numOnChargerLooks : _iConfig.numOffChargerLooks;
  PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.BehaviorUpdate.Looking", 
                "Looked %d/%d times", _dVars.numChecksSucceeded, numLooks );
  return ( _dVars.numChecksSucceeded >= numLooks );
}

void BehaviorReactToDarkness::TransitionToResponding()
{
  _dVars.state = State::Responding;
  
  // Trigger emotion
  const f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  GetBEI().GetMoodManager().TriggerEmotionEvent( _iConfig.reactionEmotion, currTime );
  
  // Reset cooldown
  GetBEI().GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::ReactToIllumination ).Reset();

  // Trigger animation reaction
  auto animAction = new TriggerAnimationAction(AnimationTrigger::ReactToDarkness);
  DelegateIfInControl(animAction, &BehaviorReactToDarkness::TransitionToGoingHome);
}
  
void BehaviorReactToDarkness::TransitionToGoingHome()
{
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_iConfig.homeFilter, locatedHomes);
  const bool hasAHome = !locatedHomes.empty();
  
  // If we're on the charger, don't know where home is, or are configured to sleep in place
  // Suggest napping to high-level AI through the whiteboard
  // NOTE "Suggesting" actually is more of "telling" the high-level AI to nap
  if( GetBEI().GetRobotInfo().IsOnChargerContacts() )
  {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.TransitionToGoingHome.SuggestSleepOnCharger",
                  "Already on charger, suggesting SleepOnCharger" );
    GetAIComp<AIWhiteboard>().OfferPostBehaviorSuggestion( PostBehaviorSuggestions::SleepOnCharger );
    CancelSelf();
  }
  else if( !hasAHome || _iConfig.sleepInPlace )
  {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.TransitionToGoingHome.SuggestSleep",
                  "Sleeping in place (may not know where home is), suggesting Sleep" );
    GetAIComp<AIWhiteboard>().OfferPostBehaviorSuggestion( PostBehaviorSuggestions::Sleep );
    CancelSelf();
  }
  // Else delegate to the FindAndGoHome behavior and wait for it to return
  else
  {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToDarkness.TransitionToGoingHome.FindAndGoHome",
                  "Attempting to go home, delegating to FindAndGoHome" );
    _dVars.state = State::GoingHome;
    DelegateIfInControl( _iConfig.goHomeBehavior.get() );
  }
}

}
}
