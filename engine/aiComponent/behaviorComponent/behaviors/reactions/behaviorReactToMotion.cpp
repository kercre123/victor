/**
 * File: behaviorReactToMotion.cpp
 *
 * Author: ross
 * Created: 2018-03-12
 *
 * Description: behavior that activates upon and turns toward motion, with options for first looking with the eyes
 *              and using either animation or procedural eyes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotion.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "engine/components/animationComponent.h"
#include "engine/components/movementComponent.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char * const kProceduralKey = "procedural";
  const char * const kEyeMotionTimeoutKey = "eyeMotionTimeout_s";
  const char * const kEyeShiftTimeoutKey = "eyeMotionShiftTimeout_s";
  const char * const kTicksInIntervalForTurnKey = "ticksInIntervalForTurn";
  const char * const kTurnIntervalSizeKey = "turnIntervalSize";
  const char * const kProceduralTurnAngleKey = "procTurnAngle_deg";
  const char * const kProceduralHeadAngleKey = "procHeadAngle_deg";
  
  const std::string kEyeMotionLayer = "reactToMotionEyes";
  const char * const kDebugName = "BehaviorReactToMotion";
  // wait this long after turning before looking for motion. this both helps make sure the camera is
  // steady before looking for motion, and elongates the time spent in this state
  const float kTurnTimeBuffer = 0.25f;
  
  CONSOLE_VAR(bool, kTurnFirst, "BehaviorReactToMotion", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::InstanceConfig::InstanceConfig( const Json::Value& config )
{
  eyeTimeout_s = JsonTools::ParseFloat( config, kEyeMotionTimeoutKey, kDebugName );
  eyeShiftTimeout_s = JsonTools::ParseFloat( config, kEyeShiftTimeoutKey, kDebugName );
  DEV_ASSERT( eyeTimeout_s >= eyeShiftTimeout_s, "eyeMotionTimeout must be >= eyeMotionShiftTimeout");
  
  ticksInIntervalForTurn = JsonTools::ParseUInt32( config, kTicksInIntervalForTurnKey, kDebugName );
  turnIntervalSize = JsonTools::ParseUInt32( config, kTurnIntervalSizeKey, kDebugName );
  
  procedural = false;
  JsonTools::GetValueOptional( config, kProceduralKey, procedural );
  if( procedural ) {
    procHeadAngle_deg = JsonTools::ParseFloat( config, kProceduralHeadAngleKey, kDebugName );
    procTurnAngle_deg = JsonTools::ParseFloat( config, kProceduralTurnAngleKey, kDebugName );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::DynamicVariables::Reset()
{
  state = State::Invalid;
  lookingDirection = MotionArea::None;
  proceduralEyeShiftActive = false;
  animationEyeShiftActive = false;
  proceduralHeadUp = false;
  timeFinishedTurning_s = -1.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::DynamicVariables::DynamicVariables()
{
  Reset();
  inMotion = false; // this one is persistent
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::AreaCondition::AreaCondition(MotionArea a,
                                                    unsigned int intervalSize,
                                                    Json::Value& config,
                                                    const std::string& ownerDebugLabel)
  : area( a )
  , condition( new ConditionMotionDetected(config) )
{
  condition->SetOwnerDebugLabel( ownerDebugLabel );
  historyBuff.Reset( intervalSize );
  for( size_t i = 0; i < intervalSize; ++i ) {
    historyBuff.push_back(false);
  }
  Reset();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::AreaCondition::Reset()
{
  sawMotionLastTick = false;
  motionLevel = -1.0f;
  intervalCount = 0;
  
  for( size_t i = 0; i < historyBuff.size(); ++i ) {
    historyBuff[i] = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::BehaviorReactToMotion(const Json::Value& config)
  : ICozmoBehavior( config )
  , _iConfig( config )
{
  MakeMemberTunable( _iConfig.ticksInIntervalForTurn, "ticksInIntervalForTurn" );
  MakeMemberTunable( _iConfig.procHeadAngle_deg, "procHeadAngle_deg" );
  MakeMemberTunable( _iConfig.procTurnAngle_deg, "procTurnAngle_deg" );
  MakeMemberTunable( _iConfig.eyeTimeout_s, "eyeTimeout_s" );
  MakeMemberTunable( _iConfig.eyeShiftTimeout_s, "eyeShiftTimeout_s" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::~BehaviorReactToMotion()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToMotion::WantsToBeActivatedBehavior() const
{
  for( const auto& motionCondition : _dVars.motionConditions ) {
    if( motionCondition.sawMotionLastTick ) {
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kProceduralKey,
    kEyeMotionTimeoutKey,
    kEyeShiftTimeoutKey,
    kTicksInIntervalForTurnKey,
    kTurnIntervalSizeKey,
    kProceduralTurnAngleKey,
    kProceduralHeadAngleKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::InitBehavior()
{
  Json::Value config;
  config["conditionType"] = "MotionDetected";
  config["motionLevel"] = "Low";
  
  config["motionArea"] = "Left";
  _dVars.motionConditions.emplace_back( MotionArea::Left, _iConfig.turnIntervalSize, config, GetDebugLabel() );
  _dVars.motionConditions.back().condition->Init( GetBEI() );
  config["motionArea"] = "Right";
  _dVars.motionConditions.emplace_back( MotionArea::Right, _iConfig.turnIntervalSize, config, GetDebugLabel() );
  _dVars.motionConditions.back().condition->Init( GetBEI() );
  config["motionArea"] = "Top";
  _dVars.motionConditions.emplace_back( MotionArea::Top, _iConfig.turnIntervalSize, config, GetDebugLabel() );
  _dVars.motionConditions.back().condition->Init( GetBEI() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::OnBehaviorEnteredActivatableScope()
{
  for( const auto& condPair : _dVars.motionConditions ) {
    condPair.condition->SetActive( GetBEI(), true );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::OnBehaviorLeftActivatableScope()
{
  for( const auto& condPair : _dVars.motionConditions ) {
    condPair.condition->SetActive( GetBEI(), false );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars.Reset();
  
  // get area with the most motion
  MotionArea mostMotionArea = GetAreaWithMostMotion();
  
  // transition (either an animation or action)
  if( mostMotionArea != MotionArea::None ) {
    // todo: once we decide whether eyes should precede turns, and whether to use animations vs procedural,
    // simplify all this logic
    if( !kTurnFirst || GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
      if( _iConfig.procedural ) {
        TransitionToProceduralEyes( mostMotionArea );
      } else {
        TransitionToEyeAnimation( mostMotionArea );
      }
    } else {
      _dVars.lookingDirection = mostMotionArea;
      _dVars.animationEyeShiftActive = true;
      if( _iConfig.procedural ) {
        TransitionToProceduralTurn();
      } else {
        TransitionToTurnAnimation();
      }
    }
  } else {
    // can happen in unit tests
    PRINT_NAMED_WARNING( "BehaviorReactToMotion.OnBehaviorActivated.NoMotionArea",
                         "Motion was detected, but the area is unknown" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::OnBehaviorDeactivated()
{
  if( _dVars.animationEyeShiftActive ) {
    // an animation has set eye direction, and we need to undo it
    AnimationTrigger trigger = AnimationTrigger::Count;
    if( _dVars.lookingDirection == MotionArea::Left ) {
      trigger = AnimationTrigger::ReactToMotionLeftGetout;
    } else if( _dVars.lookingDirection == MotionArea::Right ) {
      trigger = AnimationTrigger::ReactToMotionRightGetout;
    } else if( _dVars.lookingDirection == MotionArea::Top ) {
      trigger = AnimationTrigger::ReactToMotionUpGetout;
    }
    if( trigger != AnimationTrigger::Count ) {
      PlayEmergencyGetOut( trigger );
    }
  } else if( _dVars.proceduralEyeShiftActive ) {
    GetBEI().GetAnimationComponent().RemoveEyeShift( kEyeMotionLayer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::BehaviorUpdate() 
{
  UpdateConditions();
  
  const bool motorsMoving = GetBEI().GetRobotInfo().GetMoveComponent().IsMoving();
  const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();
  if( motorsMoving || pickedUp ) {
    _dVars.inMotion = true;
    
    // if running and the robot is picked up, stop. don't worry about if running and motorsMoving,
    // since the behavior handles that by tracking _dVars.state.
    if( pickedUp && IsActivated() ) {
      const bool revertHead = false;
      TransitionToCompleted( revertHead );
      return;
    }
  } else {
    if( _dVars.inMotion ) {
      // reset accumulated values
      for( auto& elem : _dVars.motionConditions ) {
        elem.Reset();
      }
    }
    _dVars.inMotion = false;
  }
  
  if( !IsActivated() ) {
    return;
  }
  
  if( _dVars.state == State::Invalid ) {
    CancelSelf();
    return;
  }
  
  float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( _dVars.state == State::EyeTracking ) {
    auto itCurrent = std::find_if( _dVars.motionConditions.begin(), _dVars.motionConditions.end(), [this](const auto& x){
      return x.area == _dVars.lookingDirection;
    });
    if( !ANKI_VERIFY( (itCurrent != _dVars.motionConditions.end()) && (itCurrent->lastMotionTime_s >= 0.0f),
                      "BehaviorReactToMotion.BehaviorUpdate.InvalidMotion",
                      "No motion in looking direction" ) )
    {
      TransitionToCompleted();
      return;
    }
    
    // if enough time has passed without motion, time out and exit
    const float timeSinceMotion_s = currTime - itCurrent->lastMotionTime_s;
    if( timeSinceMotion_s > _iConfig.eyeTimeout_s ) {
      TransitionToCompleted();
      return;
    }
    
    // if motion has happened in a different location, and the previous location hasnt seen motion in
    // some amount of time, switch eye tracking to a different spot
    if( timeSinceMotion_s > _iConfig.eyeShiftTimeout_s ) {
      const MotionArea mostMotionArea = GetAreaWithMostMotion();
      if( mostMotionArea != MotionArea::None ) {
        if( _iConfig.procedural ) {
          TransitionToProceduralEyes( mostMotionArea );
        } else {
          TransitionToEyeAnimation( mostMotionArea );
        }
      }
    }
    
    if( !GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
      // if motion has repeatedly happened in the same spot for a while, turn towards it
      if( itCurrent->intervalCount > _iConfig.ticksInIntervalForTurn ) {
        if( _iConfig.procedural ) {
          TransitionToProceduralTurn();
        } else {
          TransitionToTurnAnimation();
        }
      }
    }
  } else if( _dVars.state == State::TurnedAndWaiting ) {
    // wait a small while after entering this state so we're completely sure we've stopped turning
    const bool lookingForMotion = (_dVars.timeFinishedTurning_s < 0.0f);
    const bool timeExpired = !lookingForMotion && (currTime - _dVars.timeFinishedTurning_s > kTurnTimeBuffer);
    if( timeExpired ) {
      // next tick it will look for motion
      _dVars.timeFinishedTurning_s = -1.0f;
    }
    if( lookingForMotion ) {
      // if there's motion, move eyes toward it
      const MotionArea mostMotionArea = GetAreaWithMostMotion();
      if( mostMotionArea != MotionArea::None ) {
        if( _iConfig.procedural ) {
          TransitionToProceduralTurn();
        } else {
          TransitionToTurnAnimation();
        }
      } else {
        // if enough time has passed without motion, time out and exit
        if( currTime - _dVars.timeFinishedTurning_s > kTurnTimeBuffer + _iConfig.eyeTimeout_s ) {
          TransitionToCompleted();
          return;
        }
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToEyeAnimation( MotionArea area )
{
  _dVars.state = State::EyeTracking;
  
  auto* action = new CompoundActionSequential();
  if( _dVars.animationEyeShiftActive ) {
    // must first play eye getout before looking in the new direction
    if( _dVars.lookingDirection == MotionArea::Left ) {
      action->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionLeftGetout ) );
    } else if( _dVars.lookingDirection == MotionArea::Right ) {
      action->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionRightGetout ) );
    } else if( _dVars.lookingDirection == MotionArea::Top ) {
      action->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionUpGetout ) );
    }
  }
  
  _dVars.lookingDirection = area;
  
  AnimationTrigger trigger = AnimationTrigger::Count;
  if( area == MotionArea::Left ) {
    trigger = AnimationTrigger::ReactToMotionLeft;
  } else if( area == MotionArea::Right ) {
    trigger = AnimationTrigger::ReactToMotionRight;
  } else { // up
    trigger = AnimationTrigger::ReactToMotionUp;
  }
  
  u8 tracksToLock = GetBEI().GetRobotInfo().IsOnChargerPlatform()
                    ? (static_cast<u8>(AnimTrackFlag::BODY_TRACK) | static_cast<u8>(AnimTrackFlag::HEAD_TRACK))
                    : static_cast<u8>(AnimTrackFlag::HEAD_TRACK);
  
  action->AddAction( new TriggerLiftSafeAnimationAction(trigger, 1, true, tracksToLock) );
  
  _dVars.animationEyeShiftActive = true;
  
  CancelDelegates(false);
  DelegateIfInControl( action );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToProceduralEyes( MotionArea area )
{
  _dVars.state = State::EyeTracking;
  _dVars.lookingDirection = area;
  
  float pixX = 0.0f;
  float pixY = 0.0f;
  if( area == MotionArea::Left ) {
    pixX = 0.5f * FACE_DISPLAY_WIDTH;
  } else if( area == MotionArea::Right ) {
    pixX = -0.5f * FACE_DISPLAY_WIDTH;
  } else if( area == MotionArea::Top ) {
    pixY = -0.4f * FACE_DISPLAY_HEIGHT;
  }
  _dVars.proceduralEyeShiftActive = true;
  // keep the eyes in place for the max time we expect to be in this state
  const TimeStamp_t duration = static_cast<TimeStamp_t>( 1.0f + _iConfig.eyeTimeout_s );
  GetBEI().GetAnimationComponent().AddOrUpdateEyeShift( kEyeMotionLayer, pixX, pixY, duration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToTurnAnimation()
{
  _dVars.state = State::Turning;
  
  // keep the eyes wherever theyre facing and turn in that direction
  
  AnimationTrigger trigger = AnimationTrigger::Count;
  if( _dVars.lookingDirection == MotionArea::Left ) {
    trigger = AnimationTrigger::ReactToMotionTurnLeft;
  } else if( _dVars.lookingDirection == MotionArea::Right ) {
    trigger = AnimationTrigger::ReactToMotionTurnRight;
  } else { // up
    trigger = AnimationTrigger::ReactToMotionTurnUp;
  }
  
  CancelDelegates(false);
  DelegateIfInControl( new TriggerLiftSafeAnimationAction(trigger), [this](const ActionResult& result){
    TransitionToTurnedAndWaiting();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToProceduralTurn()
{
  _dVars.state = State::Turning;
  
  IAction* action;
  if( _dVars.lookingDirection == MotionArea::Top ) {
    _dVars.proceduralHeadUp = true;
    action = new MoveHeadToAngleAction( DEG_TO_RAD(_iConfig.procHeadAngle_deg) );
  } else {
    bool isAbsolute = false;
    float angle = (_dVars.lookingDirection == MotionArea::Left)
                  ? _iConfig.procTurnAngle_deg
                  : -_iConfig.procTurnAngle_deg;
    auto* turnAction = new TurnInPlaceAction( DEG_TO_RAD(angle), isAbsolute);
    turnAction->SetAccel( MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2 );
    turnAction->SetMaxSpeed( MAX_BODY_ROTATION_SPEED_RAD_PER_SEC );
    action = turnAction;
  }
  
  CancelDelegates( false );
  DelegateIfInControl( action, [this](ActionResult result){
    TransitionToTurnedAndWaiting();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToTurnedAndWaiting()
{
  _dVars.timeFinishedTurning_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.state = State::TurnedAndWaiting;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::UpdateConditions()
{
  for( auto& motionCondition : _dVars.motionConditions ) {
    if( ANKI_VERIFY( motionCondition.condition != nullptr,
                    "BehaviorReactToMotion.WantsToBeActivatedBehavior.Null",
                    "motion condition is null" ) )
    {
      if( motionCondition.condition->AreConditionsMet( GetBEI() ) ) {
        const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        motionCondition.sawMotionLastTick = true;
        motionCondition.lastMotionTime_s = currTime;
        motionCondition.motionLevel = motionCondition.condition->GetLastObservedMotionLevel();
        if( !motionCondition.historyBuff.front() ) {
          motionCondition.intervalCount += 1;
        }
        motionCondition.historyBuff.push_back( true );
      } else {
        motionCondition.sawMotionLastTick = false;
        motionCondition.motionLevel = -1.0f;
        if( motionCondition.historyBuff.front() ) {
          motionCondition.intervalCount -= 1;
          DEV_ASSERT( motionCondition.intervalCount >= 0, "Shouldn't end up negative" );
        }
        motionCondition.historyBuff.push_back( false );
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotion::MotionArea BehaviorReactToMotion::GetAreaWithMostMotion() const
{
  float mostMotion = -1.0f;
  MotionArea mostMotionArea = MotionArea::None;
  for( const auto& elem : _dVars.motionConditions ) {
    if( elem.motionLevel > mostMotion ) {
      mostMotion = elem.motionLevel;
      mostMotionArea = elem.area;
    }
  }
  return mostMotionArea;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::DevAddFakeMotion()
{
  if( (GetAreaWithMostMotion() == MotionArea::None) || !WantsToBeActivatedBehavior() ) {
    MotionArea mostMotionArea = MotionArea::Left;
    for( auto& elem : _dVars.motionConditions ) {
      if( elem.area == mostMotionArea ) {
        elem.motionLevel = 1.0f;
        elem.sawMotionLastTick = true;
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotion::TransitionToCompleted(bool revertHead)
{
  _dVars.state = State::Completed;
  
  IAction* action = nullptr;
  
  if( _dVars.animationEyeShiftActive ) {
    // an animation has set eye direction, and we need to undo it
    if( _dVars.lookingDirection == MotionArea::Left ) {
      action = new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionLeftGetout );
    } else if( _dVars.lookingDirection == MotionArea::Right ) {
      action = new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionRightGetout );
    } else if( _dVars.lookingDirection == MotionArea::Top ) {
      action = new TriggerLiftSafeAnimationAction( AnimationTrigger::ReactToMotionUpGetout );
    }
    _dVars.animationEyeShiftActive = false;
  } else if( revertHead && _dVars.proceduralHeadUp ) {
    action = new MoveHeadToAngleAction( DEG_TO_RAD(0.0f) );
    _dVars.proceduralHeadUp = false;
  }
  
  if( _dVars.proceduralEyeShiftActive ) {
    GetBEI().GetAnimationComponent().RemoveEyeShift( kEyeMotionLayer );
    _dVars.proceduralEyeShiftActive = false;
  }
  
  if( action != nullptr ) {
    CancelDelegates( false );
    DelegateIfInControl( action, [this](ActionResult result){
      CancelSelf();
    });
  } else {
    CancelSelf();
  }
  
}


}
}
