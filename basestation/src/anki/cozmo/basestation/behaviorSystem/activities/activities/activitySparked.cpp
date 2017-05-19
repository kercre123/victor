/**
* File: ActivitySparked.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for handeling cozmo's "sparked" mode
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activitySparked.h"


#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPeekABoo.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"

#include "anki/cozmo/basestation/robot.h"

#include "clad/types/behaviorTypes.h"


namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kSubActivityDelegateKey        = "subActivityDelegate";

// Spark start/end params
static const char* kMinTimeConfigKey                 = "minTimeSecs";
static const char* kMaxTimeConfigKey                 = "maxTimeSecs";
static const char* kNumberOfRepetitionsConfigKey     = "numberOfRepetitions";
static const char* kBehaviorObjectiveConfigKey       = "behaviorObjective";
static const char* ksoftSparkUpgradeTriggerConfigKey = "softSparkTrigger";
static const char* kSparksSuccessTriggerKey          = "sparksSuccessTrigger";
static const char* kSparksFailTriggerKey             = "sparksFailTrigger";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersSparksChooserArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersSparksChooserArray),
              "Reaction triggers duplicate or non-sequential");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const char* kPlayingFinalAnimationLock         = "finalAnimLockReactions";
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersFinalAnimationArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersFinalAnimationArray),
              "Reaction triggers duplicate or non-sequential");

} // end namespace
  

ActivitySparked::ActivitySparked(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _robot(robot)
, _state(ChooserState::ChooserSelected)
, _timeChooserStarted(0.f)
, _currentObjectiveCompletedCount(0)
, _minTimeSecs(-1.f)
, _maxTimeSecs(-1.f)
, _numberOfRepetitions(-1)
, _switchingToHardSpark(false)
, _idleAnimationsSet(false)
, _subActivityDelegate(nullptr)
{
  ReloadFromConfig(robot, config);
  
  // be able to reset the objects that Cozmo has reacted to when a spark starts
  IBehavior* acknowledgeObjectBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::AcknowledgeObject);
  assert(dynamic_cast< BehaviorAcknowledgeObject* >(acknowledgeObjectBehavior));
  _behaviorAcknowledgeObject = static_cast< BehaviorAcknowledgeObject* >(acknowledgeObjectBehavior);
  DEV_ASSERT(nullptr != _behaviorAcknowledgeObject, "ActivitySparked.BehaviorAcknowledgeObjectNotFound");
  
  // for COZMO-8914
  IBehavior* sparksPeekABoo = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::SparksPeekABoo);
  assert(dynamic_cast< BehaviorAcknowledgeObject* >(acknowledgeObjectBehavior));
  _behaviorPeekABoo = dynamic_cast<BehaviorPeekABoo*>(sparksPeekABoo);
  DEV_ASSERT(_behaviorPeekABoo != nullptr, "ActivitySparked.BehaviorPeekABooNotFound");
  
  // grab none behavior
  Json::Value noneConfig = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);
  _behaviorNone = robot.GetBehaviorFactory().CreateBehavior(BehaviorClass::NoneBehavior, robot, noneConfig);
  
  // Listen for behavior objective achieved messages for spark repetitions counter
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::BehaviorObjectiveAchieved>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivitySparked::~ActivitySparked()
{
  _behaviorPlayAnimation = nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySparked::OnSelectedInternal()
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
    _robot.GetBodyLightComponent().StartLoopingBackpackLights(
                            kLoopingSparkLights,
                            BackpackLightSource::Behavior,
                            _bodyLightDataLocator);
    
    _idleAnimationsSet = true;
  }
  
  // Turn off reactionary behaviors that could interrupt the spark
  SmartDisableReactionsWithLock(_robot,
                                GetIDStr(),
                                kAffectTriggersSparksChooserArray);
  
  // Notify the delegate chooser if it exists
  if(_subActivityDelegate != nullptr){
    _subActivityDelegate->OnSelected(_robot);
  }
  
  // for COZMO-8914
  if(mngr.GetRequestedSpark() == UnlockId::PeekABoo){
    _behaviorPeekABoo->PeekABooSparkStarted(_maxTimeSecs);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySparked::OnDeselectedInternal()
{
  ResetLightsAndAnimations();
  

  
  // clear any custom light events set during the spark
  
  // Notify the delegate chooser if it exists
  if(_subActivityDelegate != nullptr){
    _subActivityDelegate->OnDeselected(_robot);
  }
  
  _robot.GetCubeLightComponent().StopAllAnims();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivitySparked::ReloadFromConfig(Robot& robot, const Json::Value& config)
{
  // Set animation triggers
  _softSparkUpgradeTrigger = AnimationTrigger::Count;
  JsonTools::GetValueOptional(config, ksoftSparkUpgradeTriggerConfigKey, _softSparkUpgradeTrigger);
  
  auto successTriggerString = config.get(kSparksSuccessTriggerKey,
                                         EnumToString(AnimationTrigger::SparkSuccess)).asString();
  auto failTriggerString = config.get(kSparksFailTriggerKey,
                                      EnumToString(AnimationTrigger::SparkFailure)).asString();
  
  if(successTriggerString.empty()){
    _sparksSuccessTrigger = AnimationTrigger::Count;
  }else{
    _sparksSuccessTrigger =  AnimationTriggerFromString(successTriggerString.c_str());
  }
  
  if(failTriggerString.empty()){
    _sparksFailTrigger = AnimationTrigger::Count;
  }else{
    _sparksFailTrigger = AnimationTriggerFromString(failTriggerString.c_str());
  }
  
  //Create an arbitrary animation behavior
  Json::Value playAnimConfig = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::PlayArbitraryAnim);
  _behaviorPlayAnimation = dynamic_cast<BehaviorPlayArbitraryAnim*>(
                              robot.GetBehaviorFactory().CreateBehavior(
                                BehaviorClass::PlayArbitraryAnim, robot, playAnimConfig));
  DEV_ASSERT(_behaviorPlayAnimation, "ActivitySparked.Behavior pointer not set");
  
  _minTimeSecs = JsonTools::ParseFloat(config, kMinTimeConfigKey, "Failed to parse min time");
  _maxTimeSecs = JsonTools::ParseFloat(config, kMaxTimeConfigKey, "Failed to parse max time");
  _numberOfRepetitions =  JsonTools::ParseUint8(config, kNumberOfRepetitionsConfigKey,
                                                "Failed to parse number of repetitions");
  
  _objectiveToListenFor = BehaviorObjectiveFromString(
                              config.get(kBehaviorObjectiveConfigKey,
                                         EnumToString(BehaviorObjective::Unknown)).asCString());
  
  // Construct the simple chooser delegate if one is specified
  const Json::Value& subActivityDelegate = config[kSubActivityDelegateKey];
  if(!subActivityDelegate.isNull()){
    ActivityType activityType = IActivity::ExtractActivityTypeFromConfig(subActivityDelegate);
    _subActivityDelegate = std::unique_ptr<IActivity>(
                               ActivityFactory::CreateActivity(robot,
                                                               activityType,
                                                               subActivityDelegate));
  }
  
  
  //Ensures that these values have to be set in behavior_config for all sparks
  DEV_ASSERT(FLT_GE(_minTimeSecs, 0.f) && FLT_GE(_maxTimeSecs, 0.f)
             && _numberOfRepetitions >= 0 && _softSparkUpgradeTrigger != AnimationTrigger::Count
             && _objectiveToListenFor != BehaviorObjective::Count,
             "ActivitySparked.ReloadFromConfig: At least one parameter not set");
  
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySparked::ResetLightsAndAnimations()
{
  if(_idleAnimationsSet){
    // Revert to driving anims
    _robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    _robot.GetAnimationStreamer().PopIdleAnimation();
    _robot.GetBodyLightComponent().StopLoopingBackpackLights(_bodyLightDataLocator);
    _idleAnimationsSet = false;
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivitySparked::HandleMessage(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  BehaviorObjective objectiveAchieved = msg.behaviorObjective;
  if(objectiveAchieved == _objectiveToListenFor){
    _currentObjectiveCompletedCount++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivitySparked::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily == ObjectFamily::Block || msg.objectFamily == ObjectFamily::LightCube ) {
    _observedObjectsSinceStarted.insert( msg.objectID );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivitySparked::Update(Robot& robot)
{
  const bool isCurrentBehaviorReactionary = _robot.GetBehaviorManager().CurrentBehaviorTriggeredAsReaction();
  
  // If the intro is interrupted, just continue as normal when reaction is over
  if((_state == ChooserState::ChooserSelected ||
      _state == ChooserState::PlayingSparksIntro)){
    
    if(isCurrentBehaviorReactionary){
      _state = ChooserState::UsingSimpleBehaviorChooser;
    }
  }
  
  if(_state == ChooserState::UsingSimpleBehaviorChooser
     || _state == ChooserState::WaitingForCurrentBehaviorToStop ){
    CheckIfSparkShouldEnd();
  }
  
  // If we've timed out during a reactionary behavior, skip the outro and kill the lights
  if(_state == ChooserState::WaitingForCurrentBehaviorToStop){
    if(isCurrentBehaviorReactionary){
      CompleteSparkLogic();
      ResetLightsAndAnimations();
      _state = ChooserState::EndSparkWhenReactionEnds;
    }
  }
  
  Result result = Result::RESULT_OK;
  
  if(_subActivityDelegate != nullptr){
    result = _subActivityDelegate->Update(robot);
  }
  
  return result;  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* ActivitySparked::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
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
        bestBehavior = SelectNextSparkInternalBehavior(robot, currentRunningBehavior);
      }
      break;
    }
    
    case ChooserState::UsingSimpleBehaviorChooser:
    {
      bestBehavior = SelectNextSparkInternalBehavior(robot, currentRunningBehavior);
      break;
    }
    case ChooserState::WaitingForCurrentBehaviorToStop:
    {
      if(currentRunningBehavior != nullptr
         && currentRunningBehavior->IsRunning()){
        // wait for the current behavior to end
        bestBehavior = SelectNextSparkInternalBehavior(robot, currentRunningBehavior);
        break;
      }else{
        const bool isSoftSpark = mngr.IsActiveSparkSoft();
        
        // Set the animation behavior either to play the outro or with a placeholder for this tick
        if(!isSoftSpark && !mngr.DidGameRequestSparkEnd()){
          
          // Play different animations based on whether cozmo timed out or completed his desired reps
          std::vector<AnimationTrigger> getOutAnims;
          if(_currentObjectiveCompletedCount >= _numberOfRepetitions){
            if(_sparksSuccessTrigger != AnimationTrigger::Count){
              getOutAnims.push_back(_sparksSuccessTrigger);
            }
            
            // make sure we don't immediately play frustration upon ending a spark successfully
            _robot.GetMoodManager().TriggerEmotionEvent("SuccessfulSpark", MoodManager::GetCurrentTimeInSeconds());
            
          }else if(_sparksFailTrigger != AnimationTrigger::Count){
            getOutAnims.push_back(_sparksFailTrigger);
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* ActivitySparked::SelectNextSparkInternalBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  IBehavior* bestBehavior = nullptr;
  // If the spark has specified an alternate chooser, call
  // its choose next behavior here
  if(_subActivityDelegate == nullptr){
    bestBehavior = IActivity::ChooseNextBehavior(robot, currentRunningBehavior);
  }else{
    bestBehavior = _subActivityDelegate->
                      ChooseNextBehavior(robot,currentRunningBehavior);
  }
  
  return bestBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySparked::CompleteSparkLogic()
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
                           {{DDATA, std::to_string( _observedObjectsSinceStarted.size()).c_str()}},
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
    //Allow new activity to be chosen if we haven't received any updates from the user or switching to same spark
    if(mngr.GetActiveSpark() == mngr.GetRequestedSpark()){
      mngr.SetRequestedSpark(UnlockId::Count, false);
    }
    
    if(!mngr.IsActiveSparkSoft()){
      // Notify the game that the spark ended with some success state
      ExternalInterface::HardSparkEndedByEngine sparkEnded;
      sparkEnded.success = completedObjectives;
      _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::HardSparkEndedByEngine>(sparkEnded);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySparked::CheckIfSparkShouldEnd()
{
  BehaviorManager& mngr = _robot.GetBehaviorManager();
  const IBehavior* currentRunningBehavior = mngr.GetCurrentBehavior();
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Behaviors with _numberOfRepetitions == 0 will always wait until max time and then play success outro
  const bool minTimeAndRepetitions = FLT_GE(currentTime_s, _timeChooserStarted + _minTimeSecs)
  && (_numberOfRepetitions != 0 && _currentObjectiveCompletedCount >= _numberOfRepetitions);
  const bool maxTimeout = FLT_GE(currentTime_s, _timeChooserStarted + _maxTimeSecs)
  && currentRunningBehavior != nullptr && currentRunningBehavior->GetRequiredUnlockID() != mngr.GetActiveSpark() ;
  const bool gameRequestedSparkEnd = mngr.DidGameRequestSparkEnd();
  
  // Transitioning out of spark to freeplay  - end current spark elegantly
  if(_state == ChooserState::UsingSimpleBehaviorChooser
     && (minTimeAndRepetitions || maxTimeout || gameRequestedSparkEnd))
  {
    ResetLightsAndAnimations();
    mngr.RequestCurrentBehaviorEndOnNextActionComplete();
    _state = ChooserState::WaitingForCurrentBehaviorToStop;
    
    // Make sure we don't interrupt the final stage animation if we see a cube
    SmartDisableReactionsWithLock(_robot,
                                  kPlayingFinalAnimationLock,
                                  kAffectTriggersFinalAnimationArray);
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
