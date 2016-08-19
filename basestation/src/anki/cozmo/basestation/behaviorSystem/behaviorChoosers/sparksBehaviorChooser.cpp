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
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

static const char* kMinTimeConfigKey                 = "minTimeSecs";
static const char* kMaxTimeConfigKey                 = "maxTimeSecs";
static const char* kNumberOfRepetitionsConfigKey     = "numberOfRepetitions";
static const char* kBehaviorObjectiveConfigKey       = "behaviorObjective";
static const char* ksoftSparkUpgradeTriggerConfigKey = "softSparkTrigger";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SparksBehaviorChooser::SparksBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser(robot, config)
  , _state(ChooserState::ChooserSelected)
  , _isLiftTrackLocked(false)
  , _timeChooserStarted(0.f)
  , _currentObjectiveCompletedCount(0)
  , _minTimeSecs(-1.f)
  , _maxTimeSecs(-1.f)
  , _numberOfRepetitions(-1)
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
  
  if(currentRunningBehavior != nullptr
     && _state == ChooserState::UsingSimpleBehaviorChooser)
  {
    //Check min time and number of repetitions for sparks
    if(FLT_GE(currentTime, _timeChooserStarted + _minTimeSecs)
       && _currentObjectiveCompletedCount >= _numberOfRepetitions)
    {
      robot.GetBehaviorManager().RequestCurrentBehaviorEndImmediately("Sparks repetition count met"); // TODO: Change to stop acting
      _state = ChooserState::WaitingForCurrentBehaviorToStop;
    }
    
    
    //Enforce max timer for sparks
    if(FLT_GE(currentTime, _timeChooserStarted + _maxTimeSecs))
    {
      robot.GetBehaviorManager().RequestCurrentBehaviorEndImmediately("Sparks max time hit"); // TODO: Change to stop acting
      _state = ChooserState::WaitingForCurrentBehaviorToStop;
    }
    
    //Check to see if user has exited out of spark from unity side
    if(robot.GetBehaviorManager().DidGameRequestSparkEnd())
    {
      robot.GetBehaviorManager().RequestCurrentBehaviorEndImmediately("Sparks Game requested spark end"); // TODO: Change to stop acting
      _state = ChooserState::WaitingForCurrentBehaviorToStop;
    }
    
    // If transitioning from soft spark to actual spark (same spark type), immediately cancel
    if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
       && robot.GetBehaviorManager().IsActiveSparkSoft() == true
       && robot.GetBehaviorManager().IsRequestedSparkSoft() != true)
    {
      robot.GetBehaviorManager().RequestCurrentBehaviorEndImmediately("Sparks transition soft spark to hard spark of same type");
      _state = ChooserState::PlayingSparksOutro;
    }
  } // end currentRunningBehavior != nullptr
  
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
      
      // Ensure the robot doesn't throw a block down
      if(robot.IsCarryingObject()){
        robot.GetMoveComponent().LockTracks((u8)AnimTrackFlag::LIFT_TRACK);
        _isLiftTrackLocked = true;
      }
      
      _behaviorPlayAnimation->SetAnimationTrigger(introAnim, 1);
      bestBehavior = _behaviorPlayAnimation;
      _state = ChooserState::PlayingSparksIntro;
      
      break;
    }
    case ChooserState::PlayingSparksIntro:
    {
      if(currentRunningBehavior != nullptr){
        bestBehavior = _behaviorPlayAnimation;
      }else{
        _state = ChooserState::UsingSimpleBehaviorChooser;
        bestBehavior = BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
        
        //Unlock the track if it was locked
        // Ensure the robot doesn't throw a block down
        if(_isLiftTrackLocked ){
          robot.GetMoveComponent().UnlockTracks((u8)AnimTrackFlag::LIFT_TRACK);
          _isLiftTrackLocked = false;
        }
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
        
        // Ensure the robot doesn't throw a block down
        if(robot.IsCarryingObject()){
          robot.GetMoveComponent().LockTracks((u8)AnimTrackFlag::LIFT_TRACK);
          _isLiftTrackLocked = true;
        }
        
        bestBehavior = _behaviorPlayAnimation;
        _state = ChooserState::PlayingSparksOutro;
        break;
      }
    }
    case ChooserState::PlayingSparksOutro:
    {
      bestBehavior = _behaviorPlayAnimation;
      if(currentRunningBehavior == nullptr){
        //Notify the game that the spark is over
        robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::SparkEnded>();
        
        //Allow new goal to be chosen if we haven't recieved any updates from the user
        if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
           && !robot.GetBehaviorManager().DidGameRequestSparkEnd()){
          robot.GetBehaviorManager().SetRequestedSpark(UnlockId::Count, false);
        }
        
        //Unlock the track if it was locked
        // Ensure the robot doesn't throw a block down
        if(_isLiftTrackLocked ){
          robot.GetMoveComponent().UnlockTracks((u8)AnimTrackFlag::LIFT_TRACK);
          _isLiftTrackLocked = false;
        }
        
        bestBehavior = _behaviorNone;
      }
      break;
    }
  } // end switch(_state)
  
  return bestBehavior;
}
  
} // namespace Cozmo
} // namespace Anki
