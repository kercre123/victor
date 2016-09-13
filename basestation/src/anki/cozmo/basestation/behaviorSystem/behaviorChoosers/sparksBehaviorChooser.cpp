/**
 * File: sparksBehaviorChooser.cpp
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Class for handling playing sparks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "SparksBehaviorChooser.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

static const char* kMinTimeConfigKey                 = "minTimeSecs";
static const char* kMaxTimeConfigKey                 = "maxTimeSecs";
static const char* kNumberOfRepetitionsConfigKey     = "numberOfRepetitions";
static const char* kBehaviorObjectiveConfigKey       = "behaviorObjective";
static const char* ksoftSparkUpgradeTriggerConfigKey = "softSparkTrigger";
static const float kMaxPlayingOutroTimeout           = 10.0f;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SparksBehaviorChooser::SparksBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser(robot, config)
  , _state(ChooserState::ChooserSelected)
  , _timeChooserStarted(0.f)
  , _currentObjectiveCompletedCount(0)
  , _minTimeSecs(-1.f)
  , _maxTimeSecs(-1.f)
  , _numberOfRepetitions(-1)
  , _switchingSoftToHardSpark(false)
  , _idleAnimationsSet(false)
{
  ReloadFromConfig(robot, config);

  // Listen for behavior objective achieved messages for spark repetitions counter
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::BehaviorObjectiveAchieved>();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SparksBehaviorChooser::~SparksBehaviorChooser()
{
  _behaviorPlayAnimation = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result SparksBehaviorChooser::ReloadFromConfig(Robot& robot, const Json::Value& config)
{
  BaseClass::ReloadFromConfig(robot, config);
  _softSparkUpgradeTrigger = AnimationTrigger::Count;
  
  //Create an arbitrary animation behavior
  _behaviorPlayAnimation = dynamic_cast<BehaviorPlayArbitraryAnim*>(robot.GetBehaviorFactory().CreateBehavior(BehaviorType::PlayArbitraryAnim, robot, Json::Value()));
  ASSERT_NAMED(_behaviorPlayAnimation, "SparksBehaviorChooser.Behavior pointer not set");
  
  _minTimeSecs = JsonTools::ParseFloat(config, kMinTimeConfigKey, "Failed to parse min time");
  _maxTimeSecs = JsonTools::ParseFloat(config, kMaxTimeConfigKey, "Failed to parse max time");
  _numberOfRepetitions =  JsonTools::ParseUint8(config, kNumberOfRepetitionsConfigKey, "Failed to parse number of repetitions");
  JsonTools::GetValueOptional(config, ksoftSparkUpgradeTriggerConfigKey, _softSparkUpgradeTrigger);
  
  _objectiveToListenFor = BehaviorObjectiveFromString(config.get(kBehaviorObjectiveConfigKey, EnumToString(BehaviorObjective::Unknown)).asCString());
    
  //Ensures that these values have to be set in behavior_config for all sparks
  ASSERT_NAMED(FLT_GE(_minTimeSecs, 0) && FLT_GE(_maxTimeSecs, 0)
               && _numberOfRepetitions >= 0 && _softSparkUpgradeTrigger != AnimationTrigger::Count
               && _objectiveToListenFor != BehaviorObjective::Count,
               "SparksBehaviorChooser.ReloadFromConfig: At least one parameter not set");
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SparksBehaviorChooser::OnSelected()
{
  _timeChooserStarted = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _currentObjectiveCompletedCount = 0;
  _state = ChooserState::ChooserSelected;
  _switchingSoftToHardSpark = false;
  _timePlayingOutroStarted = 0;
  _idleAnimationsSet = false;
  

  static const BackpackLights kLoopingSparkLights = {
    .onColor                = {{NamedColors::BLACK, NamedColors::WHITE, NamedColors::WHITE, NamedColors::WHITE, NamedColors::BLACK}},
    .offColor               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{0,360,360,360,0}},
    .offPeriod_ms           = {{0,1110,1110,1110,0}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,0,120,240,0}}
  };
  
  
  if(_robot.GetBehaviorManager().IsRequestedSparkSoft()){
    // Set the idle driving animations to sparks driving anims
    _robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::SparkDrivingStart,
                                                               AnimationTrigger::SparkDrivingLoop,
                                                               AnimationTrigger::SparkDrivingStop});
    _robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::SparkIdle);
    _robot.GetLightsComponent().StartLoopingBackpackLights(kLoopingSparkLights);

    _idleAnimationsSet = true;
  }
  
  // Turn off reactionary behaviors that could interrupt the spark
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, false);
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, false);
}
  
void SparksBehaviorChooser::OnDeselected()
{
  if(_idleAnimationsSet){
    // Revert to driving anims
    _robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    _robot.GetAnimationStreamer().PopIdleAnimation();
    _idleAnimationsSet = false;
    
    _robot.GetLightsComponent().StopLoopingBackpackLights();
  }
  
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, true);
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void SparksBehaviorChooser::HandleMessage(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  BehaviorObjective objectiveAchieved = msg.behaviorObjective;
  if(objectiveAchieved == _objectiveToListenFor){
    _currentObjectiveCompletedCount++;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* SparksBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  IBehavior* bestBehavior = nullptr;
  TimeStamp_t currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // This is checked first to ensure we switch over to the new
  // chooser state during the same tick and avoid another behavior starting
  if(_state == ChooserState::UsingSimpleBehaviorChooser){
    CheckIfSparkShouldEnd(robot);
  }
  
  // Handle behavior selection based on current state
  switch(_state){
    case ChooserState::ChooserSelected:
    {
      const bool isSoftSpark = robot.GetBehaviorManager().IsActiveSparkSoft();
      AnimationTrigger introAnim;
      if(isSoftSpark){
        introAnim = _softSparkUpgradeTrigger;
      }else{
        introAnim = AnimationTrigger::SparkGetIn;
      }
      
      _behaviorPlayAnimation->SetAnimationTrigger(introAnim, 1);
      bestBehavior = _behaviorPlayAnimation;
      _state = ChooserState::PlayingSparksIntro;
      break;
    }
    case ChooserState::PlayingSparksIntro:
    {
      if(currentRunningBehavior != nullptr
         && currentRunningBehavior->IsRunning()){
        bestBehavior = _behaviorPlayAnimation;
      }else{
        _state = ChooserState::UsingSimpleBehaviorChooser;
        bestBehavior = BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
      }
      break;
    }

    case ChooserState::UsingSimpleBehaviorChooser:
    {
      bestBehavior = BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
      break;
    }
    case ChooserState::WaitingForCurrentBehaviorToStop:
    {
      if(currentRunningBehavior != nullptr
         && currentRunningBehavior->IsRunning()){
        // wait for the current behavior to end
        bestBehavior = BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
        break;
      }else{
        const bool isSoftSpark = robot.GetBehaviorManager().IsActiveSparkSoft();
        
        // Set the animation behavior either to play the outro or with a placeholder for this tick
        if(!isSoftSpark && !robot.GetBehaviorManager().DidGameRequestSparkEnd()){
          // Play different animations based on whether cozmo timed out or completed his desired reps
          if(_currentObjectiveCompletedCount >= _numberOfRepetitions){
            _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::SparkGetOutSuccess, 1);
          }else{
            _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::SparkGetOutFailure, 1);
          }
          
        }else{
          _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::Count, 1);
        }
        
        bestBehavior = _behaviorPlayAnimation;
        _state = ChooserState::PlayingSparksOutro;
        break;
      }
    }
    case ChooserState::PlayingSparksOutro:
    {
      bestBehavior = _behaviorPlayAnimation;
      if(currentRunningBehavior == nullptr || !currentRunningBehavior->IsRunning()){
        // Notify the game that the spark is over unless the UI has already updated for a soft->hard spark switch
        if(!_switchingSoftToHardSpark){
          robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::SparkEnded>();
        }
        
        //Allow new goal to be chosen if we haven't recieved any updates from the user or switching to same spark
        if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
           && !robot.GetBehaviorManager().DidGameRequestSparkEnd()
           && !_switchingSoftToHardSpark){
          robot.GetBehaviorManager().SetRequestedSpark(UnlockId::Count, false);
        }
        
        bestBehavior = _behaviorNone;
      }
      break;
    }
  } // end switch(_state)
  
  // Ensure that the spark timesout eventually
  if(_state == ChooserState::PlayingSparksOutro
     && _timePlayingOutroStarted == 0){
    _timePlayingOutroStarted = currentTime;
  }else if(_state == ChooserState::PlayingSparksOutro){
    ASSERT_NAMED_EVENT(_timePlayingOutroStarted + kMaxPlayingOutroTimeout > currentTime,
                       "SparksBehaviorChooser.ChooseNextBehavior.OutroTimeout",
                       "Outro has not finished after %u seconds", currentTime - _timePlayingOutroStarted);
    
  }
  
  return bestBehavior;
}
  
  
void SparksBehaviorChooser::CheckIfSparkShouldEnd(Robot& robot)
{
  
  TimeStamp_t currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const BehaviorManager& mngr = robot.GetBehaviorManager();
  
  // Behaviors with _numberOfRepetions == 0 will always wait until max time and then play success outro
  const bool minTimeAndRepetitions = FLT_GE(currentTime, _timeChooserStarted + _minTimeSecs)
                                                && (_numberOfRepetitions != 0 && _currentObjectiveCompletedCount >= _numberOfRepetitions);
  const bool maxTimeout = FLT_GE(currentTime, _timeChooserStarted + _maxTimeSecs);
  const bool gameRequestedSparkEnd = mngr.DidGameRequestSparkEnd();
  
  // Transitioning out of spark to freeplay  - end current spark elegantly
  if(minTimeAndRepetitions || maxTimeout || gameRequestedSparkEnd)
  {
    robot.GetBehaviorManager().RequestCurrentBehaviorEndOnNextActionComplete();
    _state = ChooserState::WaitingForCurrentBehaviorToStop;
  }else{
    // Transitioning directly between sparks - end current spark immediately
    if(mngr.GetRequestedSpark() != UnlockId::Count){
      const bool softSparkToSoftSpark = mngr.GetActiveSpark() != mngr.GetRequestedSpark();
      const bool softSparkToHardSpark = mngr.IsActiveSparkSoft() && !mngr.IsRequestedSparkSoft();
      
      if(softSparkToSoftSpark || softSparkToHardSpark){
        robot.GetBehaviorManager().RequestCurrentBehaviorEndImmediately("Sparks transition soft spark to soft spark");
        _switchingSoftToHardSpark = true;
        _state = ChooserState::PlayingSparksOutro;
      }
    } // end (mngr.GetRequestedSpark() != UnlockID::Count)
  }
}

  
  
} // namespace Cozmo
} // namespace Anki
