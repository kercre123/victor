/**
 * File: behaviorReactToIllumination.cpp
 *
 * Author: Humphrey Hu
 * Created: 2018-06-03
 *
 * Description: Behavior to react when the lights go out
 * 
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToIllumination.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
// #include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIlluminationDetected.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

#include <iterator>

namespace Anki {
namespace Cozmo {

namespace {
static const char* kLookAheadAngleKey = "lookHeadAngle";
static const char* kLookWaitTimeKey   = "lookWaitTime";
static const char* kLookTurnAngle     = "lookTurnAngle";
static const char* kNumChecksKey      = "numLookChecks";
static const char* kPositiveConfigKey = "positiveCondition";
static const char* kNegativeConfigKey = "negativeCondition";
static const char* kEmotionKey        = "reactionEmotion";
}

BehaviorReactToIllumination::BehaviorReactToIllumination( const Json::Value& config )
: ICozmoBehavior( config )
{
  _iConfig.positiveConfig = config[kPositiveConfigKey];
  _iConfig.negativeConfig = config[kNegativeConfigKey];
  _iConfig.lookHeadAngle = DEG_TO_RAD( JsonTools::ParseFloat( config, kLookAheadAngleKey,
                                                              "BehaviorReactToIllumination.Constructor" ) );
  _iConfig.lookWaitTime_s = JsonTools::ParseFloat( config, kLookWaitTimeKey,
                                                   "BehaviorReactToIllumination.Constructor" );
  _iConfig.lookTurnAmount = DEG_TO_RAD( JsonTools::ParseFloat( config, kLookTurnAngle,
                                                               "BehaviorReactToIllumination.Constructor" ) );
  _iConfig.numChecks = JsonTools::ParseUInt32( config, kNumChecksKey,
                                               "BehaviorReactToIllumination.Constructor" );
  _iConfig.reactionEmotion = JsonTools::ParseString( config, kEmotionKey,
                                                     "BehaviorReactToIllumination.Constructor" );
}

bool BehaviorReactToIllumination::WantsToBeActivatedBehavior() const
{
  return _dVars.positiveCondition->AreConditionsMet( GetBEI() );
}

void BehaviorReactToIllumination::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
  const char* list[] = {
    kLookAheadAngleKey,
    kLookWaitTimeKey,
    kLookTurnAngle,
    kNumChecksKey,
    kPositiveConfigKey,
    kNegativeConfigKey,
    kEmotionKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

void BehaviorReactToIllumination::InitBehavior()
{
  _dVars.positiveCondition.reset( new ConditionIlluminationDetected( _iConfig.positiveConfig ) );
  _dVars.positiveCondition->SetOwnerDebugLabel( GetDebugLabel() );
  _dVars.positiveCondition->Init( GetBEI() );
  _dVars.negativeCondition.reset( new ConditionIlluminationDetected( _iConfig.negativeConfig ) );
  _dVars.negativeCondition->SetOwnerDebugLabel( GetDebugLabel() );
  _dVars.negativeCondition->Init( GetBEI() );
  _dVars.Reset();
}

void BehaviorReactToIllumination::OnBehaviorActivated()
{
  // Perform look around to confirm that condition is met
  _dVars.positiveCondition->Reset();
  _dVars.negativeCondition->Reset();
  _dVars.Reset();
}

void BehaviorReactToIllumination::OnBehaviorEnteredActivatableScope()
{
  _dVars.positiveCondition->SetActive( GetBEI(), true );
  _dVars.negativeCondition->SetActive( GetBEI(), true );
}

void BehaviorReactToIllumination::OnBehaviorLeftActivatableScope()
{
  _dVars.negativeCondition->SetActive( GetBEI(), false );
  _dVars.positiveCondition->SetActive( GetBEI(), false );
}

void BehaviorReactToIllumination::BehaviorUpdate()
{
  if( !IsActivated() ) { return; }

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
        PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.BehaviorUpdate.Timeout", "");
        CancelSelf();
      }

      // Illumination check succeeded, increment counter
      if( _dVars.positiveCondition->AreConditionsMet( GetBEI() ) )
      {
        PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.BehaviorUpdate.PositiveMet", "" );
        _dVars.numChecksSucceeded++;
        _dVars.waitStartTime = currTime;
      }
      // Illumination check failed
      else if( _dVars.negativeCondition->AreConditionsMet( GetBEI() ) )
      {
        PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.BehaviorUpdate.NegativeMet", "" );
        CancelSelf();
      }
      // Not sure yet, keep waiting
      else
      {
        break;
      }

      // If on charger, only look once?
      // TODO: Maybe tilt head instead of turning
      bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
      if( onCharger || _dVars.numChecksSucceeded >= _iConfig.numChecks )
      {
        PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.BehaviorUpdate.RunningTriggers", "" );
        RunTriggers();
        CancelSelf();
      }
      else
      {
        PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.BehaviorUpdate.NeedMoreChecks", 
                           "Have %d/%d checks", _dVars.numChecksSucceeded, _iConfig.numChecks );
        TransitionToTurning();
      }
      break;
    }
  }
}

void BehaviorReactToIllumination::DynamicVariables::Reset()
{
  state = State::Waiting;
  numChecksSucceeded = 0;
}

void BehaviorReactToIllumination::TransitionToTurning()
{
  _dVars.state = State::Turning;

  Util::RandomGenerator* rng = GetBEI().GetRobotInfo().GetContext()->GetRandom();
  double headAngle = rng->RandDblInRange( 0.0, _iConfig.lookHeadAngle );
  double turnAngle = rng->RandDblInRange( -_iConfig.lookTurnAmount, _iConfig.lookTurnAmount );
  PRINT_NAMED_DEBUG( "BehaviorReactToIllumination.TransitionToTurning", 
                     "Setting head to %f and turning %f", RAD_TO_DEG(headAngle), RAD_TO_DEG(turnAngle) );

  // TODO Sample a little bit of noise to add to the angle references
  CompoundActionParallel* action = new CompoundActionParallel();
  action->AddAction( new MoveHeadToAngleAction( headAngle ) );
  action->AddAction( new TurnInPlaceAction( turnAngle, false ) );
  DelegateIfInControl( action, &BehaviorReactToIllumination::TransitionToTurnedAndWaiting );
}

void BehaviorReactToIllumination::TransitionToTurnedAndWaiting()
{
  PRINT_NAMED_DEBUG("BehaviorReactToIllumination.TransitionToTurnedAndWaiting", "");
  _dVars.state = State::TurnedAndWaiting;
  _dVars.waitStartTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void BehaviorReactToIllumination::RunTriggers()
{
  // TODO Play animations?
  f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  GetBEI().GetMoodManager().TriggerEmotionEvent( _iConfig.reactionEmotion, currTime );

  GetBEI().GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::ReactToIllumination ).Reset();

}

}
}