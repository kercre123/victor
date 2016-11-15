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
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorAcknowledgeObject.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SparksBehaviorChooser::SparksBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser(robot, config)
  , _robot(robot)
  , _state(ChooserState::ChooserSelected)
  , _timeChooserStarted(0.f)
  , _currentObjectiveCompletedCount(0)
  , _minTimeSecs(-1.f)
  , _maxTimeSecs(-1.f)
  , _numberOfRepetitions(-1)
  , _switchingToHardSpark(false)
  , _idleAnimationsSet(false)
{
  ReloadFromConfig(robot, config);

  // be able to reset the objects that Cozmo has reacted to when a spark starts
  IBehavior* acknowledgeObjectBehavior = robot.GetBehaviorFactory().FindBehaviorByName("AcknowledgeObject");
  assert(dynamic_cast< BehaviorAcknowledgeObject* >(acknowledgeObjectBehavior));
  _behaviorAcknowledgeObject = static_cast< BehaviorAcknowledgeObject* >(acknowledgeObjectBehavior);
  ASSERT_NAMED( nullptr != _behaviorAcknowledgeObject, "SparksBehaviorChooser.BehaviorAcknowledgeObjectNotFound" );
  
  // Listen for behavior objective achieved messages for spark repetitions counter
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::BehaviorObjectiveAchieved>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
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
  _switchingToHardSpark = false;
  _timePlayingOutroStarted = 0;
  _idleAnimationsSet = false;
  _observedObjectsSinceStarted.clear();
  
  static const BackpackLights kLoopingSparkLights = {
    .onColors               = {{NamedColors::BLACK, NamedColors::WHITE, NamedColors::WHITE, NamedColors::WHITE, NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{0,360,360,360,0}},
    .offPeriod_ms           = {{0,1110,1110,1110,0}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,0,120,240,0}}
  };
  
  BehaviorManager& mngr = _robot.GetBehaviorManager();
  
  if(!mngr.IsRequestedSparkSoft()){
    // Set the idle driving animations to sparks driving anims
    _robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::SparkDrivingStart,
                                                               AnimationTrigger::SparkDrivingLoop,
                                                               AnimationTrigger::SparkDrivingStop});
    _robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::SparkIdle);
    _robot.GetLightsComponent().StartLoopingBackpackLights(kLoopingSparkLights);

    _idleAnimationsSet = true;
  }
  
  // Have Cozmo look up after seeing objects for the first time while sparked in case the user is trying to help
  if(_behaviorAcknowledgeObject != nullptr){
    _behaviorAcknowledgeObject->ResetReactionData();
  }
  
  // Turn off reactionary behaviors that could interrupt the spark
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, false);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, false);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToFrustration, false);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToPet, false);

}
  
void SparksBehaviorChooser::OnDeselected()
{
  BehaviorManager& mngr = _robot.GetBehaviorManager();

  if(_idleAnimationsSet){
    ResetLightsAndAnimations();
  }
  
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, true);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, true);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToFrustration, true);
  mngr.RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToPet, true);

}
  
void SparksBehaviorChooser::ResetLightsAndAnimations()
{
  // Revert to driving anims
  _robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  _robot.GetAnimationStreamer().PopIdleAnimation();
  _idleAnimationsSet = false;
  
  _robot.GetLightsComponent().StopLoopingBackpackLights();
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
template<>
void SparksBehaviorChooser::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily == ObjectFamily::Block || msg.objectFamily == ObjectFamily::LightCube ) {
    _observedObjectsSinceStarted.insert( msg.objectID );
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result SparksBehaviorChooser::Update()
{
  // If the intro is interrupted, just continue as normal when reaction is over
  if((_state == ChooserState::ChooserSelected ||
     _state == ChooserState::PlayingSparksIntro)){
    auto currentBehavior = _robot.GetBehaviorManager().GetCurrentBehavior();
    if(currentBehavior != nullptr && currentBehavior->IsReactionary()){
      _state = ChooserState::UsingSimpleBehaviorChooser;
    }
  }
  
  if(_state == ChooserState::UsingSimpleBehaviorChooser
     || _state == ChooserState::WaitingForCurrentBehaviorToStop ){
    CheckIfSparkShouldEnd();
  }
  
  // If we've timed out during a reactionary behavior, skip the outro and kill the lights
  if(_state == ChooserState::WaitingForCurrentBehaviorToStop){
    auto currentBehavior = _robot.GetBehaviorManager().GetCurrentBehavior();
    if(currentBehavior != nullptr && currentBehavior->IsReactionary()){
      CompleteSparkLogic();
      ResetLightsAndAnimations();
      _state = ChooserState::EndSparkWhenReactionEnds;
    }
  }
  
  return Result::RESULT_OK;
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* SparksBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  const BehaviorManager& mngr = _robot.GetBehaviorManager();

  IBehavior* bestBehavior = nullptr;
  
  // Handle behavior selection based on current state
  switch(_state){
    case ChooserState::ChooserSelected:
    {
      const bool isSoftSpark = mngr.IsActiveSparkSoft();
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
        const bool isSoftSpark = mngr.IsActiveSparkSoft();
        
        // Set the animation behavior either to play the outro or with a placeholder for this tick
        if(!isSoftSpark && !mngr.DidGameRequestSparkEnd()){
          
          std::vector<AnimationTrigger> getOutAnims;
          // Play different animations based on whether cozmo timed out or completed his desired reps
          if(_currentObjectiveCompletedCount >= _numberOfRepetitions){
            getOutAnims.push_back(AnimationTrigger::SparkSuccess);
            
            // make sure we don't immediately play frustration upon ending a spark successfully
            _robot.GetMoodManager().TriggerEmotionEvent("SuccessfulSpark", MoodManager::GetCurrentTimeInSeconds());
            
          }else{
            getOutAnims.push_back(AnimationTrigger::SparkFailure);
          }
          // then play standard get out
          getOutAnims.push_back(AnimationTrigger::SparkGetOut);
          
          const int numLoops = 1;
          _behaviorPlayAnimation->SetAnimationTriggers(getOutAnims, numLoops);
          
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
        bestBehavior = _behaviorNone;
        CompleteSparkLogic();
      }

      break;
    }
    case ChooserState::EndSparkWhenReactionEnds:
    {
      bestBehavior = _behaviorNone;
      break;
    }
  } // end switch(_state)
  
  return bestBehavior;
}
 
void SparksBehaviorChooser::CompleteSparkLogic()
{
  BehaviorManager& mngr = _robot.GetBehaviorManager();

  const bool completedObjectives = _numberOfRepetitions == 0 ||
                        _currentObjectiveCompletedCount >= _numberOfRepetitions;
  
  {
    // Send DAS event with results of spark using different events
    static constexpr const char* kDasSuccessEvent = "meta.upgrade_replay_success";
    static constexpr const char* kDasFailEvent    = "meta.upgrade_replay_fail";
    static constexpr const char* kDasCancelEvent  = "meta.upgrade_replay_cancel";
    static constexpr const char* kDasTimeoutEvent = "meta.upgrade_replay_timeout";
    
    const char* eventName = nullptr;

    const bool observedBlock = !_observedObjectsSinceStarted.empty();

    // user has canceled if they requested an end, or if they switched from soft to hard (they canceled the
    // soft spark to turn it into a hard spark)
    const bool userCanceled = mngr.DidGameRequestSparkEnd() || _switchingToHardSpark;
    
    if( userCanceled ) {
      eventName = kDasCancelEvent;
    }
    else if( completedObjectives ) {
      eventName = kDasSuccessEvent;
    }
    else if( observedBlock ) {
      eventName = kDasFailEvent;

      // in the failure case, also send a failure event with the number of cubes observed (useful to debugging
      // / collecting data on failures). Only broadcast for hard sparks for now
      if( ! mngr.IsActiveSparkSoft() ) {
        Anki::Util::sEvent("meta.upgrade_replay_fail_cubes_observed",
                           {{DDATA, TO_DDATA_STR( _observedObjectsSinceStarted.size() )}},
                           UnlockIdToString(mngr.GetActiveSpark()));
      }
    }
    else {
      // if we never saw a block, and didn't get our objective, then presumably we timed out (as opposed to
      // trying and failing). Note that some sparks (like pounce) don't use blocks, but they also don't fail,
      // so currently a non-issue, but could become a problem if, e.g. that spark "failed" if it never saw any
      // motion
      eventName = kDasTimeoutEvent;
    }
    
    Anki::Util::sEvent(eventName,
                       {{DDATA, mngr.IsActiveSparkSoft() ? "soft" : "hard"}},
                       UnlockIdToString(mngr.GetActiveSpark()));
  }
  
  // UI updates
  if(!mngr.DidGameRequestSparkEnd() && !_switchingToHardSpark){
    //Allow new goal to be chosen if we haven't received any updates from the user or switching to same spark
    if(mngr.GetActiveSpark() == mngr.GetRequestedSpark()){
      mngr.SetRequestedSpark(UnlockId::Count, false);
    }
    
    if(!mngr.IsActiveSparkSoft()){
      // Notify the game that the spark ended with some success state
      ExternalInterface::SparkEnded sparkEnded;
      sparkEnded.success = completedObjectives;
      _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::SparkEnded>(sparkEnded);
    }
  }
}

  
void SparksBehaviorChooser::CheckIfSparkShouldEnd()
{
  BehaviorManager& mngr = _robot.GetBehaviorManager();
  const IBehavior* currentRunningBehavior = mngr.GetCurrentBehavior();
  
  TimeStamp_t currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Behaviors with _numberOfRepetitions == 0 will always wait until max time and then play success outro
  const bool minTimeAndRepetitions = FLT_GE(currentTime, _timeChooserStarted + _minTimeSecs)
                                                && (_numberOfRepetitions != 0 && _currentObjectiveCompletedCount >= _numberOfRepetitions);
  const bool maxTimeout = FLT_GE(currentTime, _timeChooserStarted + _maxTimeSecs)
                            && currentRunningBehavior != nullptr && currentRunningBehavior->GetRequiredUnlockID() != mngr.GetActiveSpark() ;
  const bool gameRequestedSparkEnd = mngr.DidGameRequestSparkEnd();
  
  // Transitioning out of spark to freeplay  - end current spark elegantly
  if(_state == ChooserState::UsingSimpleBehaviorChooser
     && (minTimeAndRepetitions || maxTimeout || gameRequestedSparkEnd))
  {
    mngr.RequestCurrentBehaviorEndOnNextActionComplete();
    _state = ChooserState::WaitingForCurrentBehaviorToStop;
  }else{
    // Transitioning directly between sparks - end current spark immediately
    if(mngr.GetRequestedSpark() != UnlockId::Count){
      const bool softSparkToSoftSpark = mngr.GetActiveSpark() != mngr.GetRequestedSpark();
      const bool softSparkToHardSpark = mngr.IsActiveSparkSoft() && !mngr.IsRequestedSparkSoft();
      
      if(softSparkToSoftSpark || softSparkToHardSpark){
        mngr.RequestCurrentBehaviorEndImmediately("Sparks transition soft spark to soft spark");
        _switchingToHardSpark = true;
        _state = ChooserState::PlayingSparksOutro;
      }
    } // end (mngr.GetRequestedSpark() != UnlockID::Count)
  }
}

  
  
} // namespace Cozmo
} // namespace Anki
