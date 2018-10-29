/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description: Base class for all playpen related behaviors
 *              All Playpen behaviors should be written to be able to continue even after
 *              receiving unexpected things (basically conditional branches should only contain code
 *              that calls SET_RESULT) E.g. Even if camera calibration is outside our threshold we should
 *              still be able to continue running through the rest of playpen.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenTest.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

namespace {

// Set of messages that will immediately cause any playpen behaivor to end
// Unless they have explicitly subscribed to the message themselves and are expecting to see it
static const std::set<ExternalInterface::MessageEngineToGameTag> kFailureTags = {
  ExternalInterface::MessageEngineToGameTag::CliffEvent,
  ExternalInterface::MessageEngineToGameTag::RobotStopped,
  ExternalInterface::MessageEngineToGameTag::ChargerEvent,
  ExternalInterface::MessageEngineToGameTag::MotorCalibration,
  ExternalInterface::MessageEngineToGameTag::MotorAutoEnabled,
  ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged,
  ExternalInterface::MessageEngineToGameTag::UnexpectedMovement
};

// Static (shared) across all playpen behaviors
// Maps a behavior name/idStr to a vector of results it has failed/completed with
static std::map<std::string, std::vector<FactoryTestResultCode>> results;

// Hacky way of giving any playpen behavior easy access to check something that an individual behavior
// sets. This way the behavior that is checking doesn't have to dynamic cast to the individual behavior.
// Only behaviorPlaypenSoundCheck should be setting this bool
static bool receivedFFTResult = false;
}
 
IBehaviorPlaypen::IBehaviorPlaypen(const Json::Value& config)
: ICozmoBehavior(config)
{
  // Don't want to invailidate kFailureTags by std::move-ing it so make a copy and move that
  auto failureTagCopy = kFailureTags;
  ICozmoBehavior::SubscribeToTags(std::move(failureTagCopy));
}

bool IBehaviorPlaypen::WantsToBeActivatedBehavior() const
{
  _factoryTestLogger = &BehaviorPlaypenTest::GetFactoryTestLogger();
  return true;
}

void IBehaviorPlaypen::OnBehaviorActivated()
{
  // Add a timer to force the behavior to end if it runs too long
  AddTimer(PlaypenConfig::kDefaultTimeout_ms, [this](){
    PRINT_NAMED_WARNING("IBehaviorPlaypen.Timeout",
                        "Behavior %s has timed out and we are %signoring failures",
                        GetDebugLabel().c_str(),
                        (ShouldIgnoreFailures() ? "" : "NOT "));
    
    if(!ShouldIgnoreFailures())
    {
      SetResult(FactoryTestResultCode::TEST_TIMED_OUT);
    }
    else
    {
      SetResult(FactoryTestResultCode::SUCCESS);
    }
  });
  
  OnBehaviorActivatedInternal();
}
  
void IBehaviorPlaypen::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }

  // Immediately stop the behavior if the result is not success
  if(_result != FactoryTestResultCode::UNKNOWN &&
     _result != FactoryTestResultCode::SUCCESS)
  {
    CancelSelf();
    return;
  }

  // Update the timers
  for(auto& timer : _timers)
  {
    timer.Tick();
  }

  // Keep updating behavior until it completes
  if(_lastStatus != PlaypenStatus::Complete)
  {
    _lastStatus = PlaypenUpdateInternal();
  }
  
  // Finish recording touch data before letting the behavior complete
  if(_recordingTouch)
  {
    return;
  }

  if(_lastStatus != PlaypenStatus::Running)
  {
    CancelSelf();
  }
}
  
void IBehaviorPlaypen::HandleWhileActivated(const EngineToGameEvent& event)
{
  EngineToGameTag tag = event.GetData().GetTag();
  
  // If a subclass subscribed to the tag then let them handle it otherwise it is one we (the baseclass) subscribed
  // to, one of the kFailureTags
  if(_tagsSubclassSubscribeTo.count(tag) > 0)
  {
    HandleWhileActivatedInternal(event);
  }
  else
  {
    switch(tag)
    {
      case EngineToGameTag::CliffEvent:
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
        break;
      }
      case EngineToGameTag::RobotStopped:
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
        break;
      }
      case EngineToGameTag::ChargerEvent:
      {
        if(event.GetData().Get_ChargerEvent().onCharger)
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::UNEXPECTED_ON_CHARGER);
        }
        break;
      }
      case EngineToGameTag::MotorCalibration:
      {
        const auto& payload = event.GetData().Get_MotorCalibration();
        if(payload.motorID == MotorID::MOTOR_HEAD)
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::HEAD_MOTOR_CALIB_UNEXPECTED);
        }
        else if(payload.motorID == MotorID::MOTOR_LIFT)
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::LIFT_MOTOR_CALIB_UNEXPECTED);
        }
        else
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::MOTOR_CALIB_UNEXPECTED);
        }
        break;
      }
      case EngineToGameTag::MotorAutoEnabled:
      {
        const auto& payload = event.GetData().Get_MotorAutoEnabled();
        if(!payload.enabled)
        {
          if(payload.motorID == MotorID::MOTOR_HEAD)
          {
            PLAYPEN_SET_RESULT(FactoryTestResultCode::HEAD_MOTOR_DISABLED);
          }
          else if(payload.motorID == MotorID::MOTOR_LIFT)
          {
            PLAYPEN_SET_RESULT(FactoryTestResultCode::LIFT_MOTOR_DISABLED);
          }
          else
          {
            PLAYPEN_SET_RESULT(FactoryTestResultCode::MOTOR_DISABLED);
          }
        }
        break;
      }
      case EngineToGameTag::RobotOffTreadsStateChanged:
      {
        if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState != OffTreadsState::OnTreads)
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::ROBOT_PICKUP);
        }
        break;
      }
      case EngineToGameTag::UnexpectedMovement:
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::UNEXPECTED_MOVEMENT_DETECTED);
        break;
      }
      default:
      {
        return;
      }
    }
  }
}

void IBehaviorPlaypen::HandleWhileActivated(const RobotToEngineEvent& event)
{
  HandleWhileActivatedInternal(event);
}

void IBehaviorPlaypen::SubscribeToTags(std::set<EngineToGameTag>&& tags)
{
  _tagsSubclassSubscribeTo.insert(tags.begin(), tags.end());
  
  // If the caller wants to subscribe to one of the failure type tags then don't let them subscribe
  // since we already are subscribed to it and it will already call HandleWhileActivatedInternal()
  for(auto iter = tags.begin(); iter != tags.end();)
  {
    if(kFailureTags.count(*iter) > 0)
    {
      iter = tags.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  
  ICozmoBehavior::SubscribeToTags(std::move(tags));
}

void IBehaviorPlaypen::SubscribeToTags(std::set<GameToEngineTag>&& tags)
{
  ICozmoBehavior::SubscribeToTags(std::move(tags));
}

bool IBehaviorPlaypen::DelegateIfInControl(IActionRunner* action, SimpleCallback callback)
{
  // Caller is not checking the action result so fail if the action fails
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::ACTION_FAILED);
    }

    callback();
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

bool IBehaviorPlaypen::DelegateIfInControl(IActionRunner* action, ActionResultCallback callback)
{
  // Caller is checking the action result so just print a warning on failure and let the
  // caller deal with the action result
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorPlaypen.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetDebugLabel().c_str(),
                          EnumToString(result));
    }
    
    callback(result);
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

bool IBehaviorPlaypen::DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback)
{
  // Caller is checking the rca so just print a warning on failure and let the
  // caller deal with the rca
  auto callbackWrapper = [this, callback](const ExternalInterface::RobotCompletedAction& rca){
    if(rca.result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorPlaypen.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetDebugLabel().c_str(),
                          EnumToString(rca.result));
    }
    
    callback(rca);
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

void IBehaviorPlaypen::WriteToStorage(Robot& robot, 
                                      NVStorage::NVEntryTag tag,
                                      const u8* data, 
                                      size_t size,
                                      FactoryTestResultCode failureCode)
{
  if(PlaypenConfig::kWriteToStorage)
  {
    PLAYPEN_TRY(robot.GetNVStorageComponent().Write(tag, data, size),
                failureCode);
  }
  else
  {
    PRINT_CH_INFO("Behaviors", "IBehaviorPlaypen.WriteToStorage.WritingNotEnabled", 
                     "Not writing to %s",
                     EnumToString(tag));
  }
}
  
bool IBehaviorPlaypen::ShouldIgnoreFailures() const
{
  return PlaypenConfig::kIgnoreFailures;
}

void IBehaviorPlaypen::SetResult(FactoryTestResultCode result)
{
  _result = result;
  ClearTimers();
  AddToResultList(result);
}

void IBehaviorPlaypen::AddToResultList(FactoryTestResultCode result)
{
  results[GetDebugLabel()].push_back(result);
}

const std::map<std::string, std::vector<FactoryTestResultCode>>& IBehaviorPlaypen::GetAllPlaypenResults()
{
  return results;
}

void IBehaviorPlaypen::ResetAllPlaypenResults()
{
  results.clear();
}

const std::set<ExternalInterface::MessageEngineToGameTag>& IBehaviorPlaypen::GetFailureTags()
{
  return kFailureTags;
}

void IBehaviorPlaypen::ReceivedFFTResult()
{
  receivedFFTResult = true;
}

bool IBehaviorPlaypen::DidReceiveFFTResult()
{
  bool b = receivedFFTResult;
  receivedFFTResult = false;
  return b;
}

void IBehaviorPlaypen::RemoveTimer(const std::string& name)
{
  for(auto iter = _timers.begin(); iter != _timers.end(); ++iter)
  {
    if(iter->GetName() == name)
    {
      _timers.erase(iter);
      return;
    }
  }
}

void IBehaviorPlaypen::RecordTouchSensorData(Robot& robot, const std::string& nameOfData)
{
  if(PlaypenConfig::kDurationOfTouchToRecord_ms > 0)
  {
    // Start recording data to the _touchSensorValues struct
    _recordingTouch = true;
    robot.GetTouchSensorComponent().StartRecordingData(&_touchSensorValues);

    // Add a timer that will stop recording touch data and write it to log
    // This timer will not be removed from the timer vector so it must complete before the behavior is allowed to
    // complete
    AddTimer(PlaypenConfig::kDurationOfTouchToRecord_ms, [this, &robot, nameOfData](){ 
      robot.GetTouchSensorComponent().StopRecordingData();
      PLAYPEN_TRY(GetLogger().Append("TouchSensor_" + nameOfData, _touchSensorValues), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
      _touchSensorValues.data.clear();
      _recordingTouch = false;
    },
    "DontDelete");
  }
}

void IBehaviorPlaypen::ClearTimers()
{
  // Remove all timers except for the ones marked as "DontDelete"
  for(auto iter = _timers.begin(); iter < _timers.end();)
  {
    if(iter->GetName() != "DontDelete")
    {
      iter = _timers.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}
  
}
}
