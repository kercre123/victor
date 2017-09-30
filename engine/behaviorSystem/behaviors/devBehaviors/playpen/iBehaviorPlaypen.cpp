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
 *              still be able to continue running the rest through the rest of playpen.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

#include "engine/behaviorSystem/activities/activities/activityPlaypenTest.h"
#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqPlaypen.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

// Set of messages that will immediately cause any playpen behaivor to end
// Unless they have explicitly subscribed to the a message themselves and are expecting to see it
static const std::set<ExternalInterface::MessageEngineToGameTag> kFailureTags = {
  ExternalInterface::MessageEngineToGameTag::RobotState,
  ExternalInterface::MessageEngineToGameTag::CliffEvent,
  ExternalInterface::MessageEngineToGameTag::RobotStopped,
  ExternalInterface::MessageEngineToGameTag::ChargerEvent,
  ExternalInterface::MessageEngineToGameTag::MotorCalibration,
  ExternalInterface::MessageEngineToGameTag::MotorAutoEnabled,
  ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged,
  ExternalInterface::MessageEngineToGameTag::UnexpectedMovement
};

// Static (shared) across all playpen behaviors
static std::map<std::string, std::vector<FactoryTestResultCode>> results;
}
 
IBehaviorPlaypen::IBehaviorPlaypen(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  // Don't want to invailidate kFailureTags by std::move-ing it so make a copy and move that
  auto failureTagCopy = kFailureTags;
  IBehavior::SubscribeToTags(std::move(failureTagCopy));
}

bool IBehaviorPlaypen::IsRunnableInternal(const Robot& robot) const
{
  _factoryTestLogger = &ActivityPlaypenTest::GetFactoryTestLogger();
  return true;
}

Result IBehaviorPlaypen::InitInternal(Robot& robot)
{
  // Add a timer to force the behavior to end if runs too long
  AddTimer(PlaypenConfig::kDefaultTimeout_ms, [this](){
    PRINT_NAMED_WARNING("IBehaviorPlaypen.Timeout",
                        "Behavior %s has timed out and we are %s ignoring failures",
                        GetIDStr().c_str(),
                        (ShouldIgnoreFailures() ? "" : "NOT"));
    
    if(!ShouldIgnoreFailures())
    {
      SetResult(FactoryTestResultCode::TEST_TIMED_OUT);
    }
    else
    {
      SetResult(FactoryTestResultCode::SUCCESS);
    }
  });
  
  return InternalInitInternal(robot);
}
  
BehaviorStatus IBehaviorPlaypen::UpdateInternal(Robot& robot)
{
  if(_result != FactoryTestResultCode::UNKNOWN &&
     _result != FactoryTestResultCode::SUCCESS)
  {
    return BehaviorStatus::Failure;
  }

  // Update the timers
  for(auto& timer : _timers)
  {
    timer.Tick();
  }
  
  return InternalUpdateInternal(robot);
}
  
void IBehaviorPlaypen::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  EngineToGameTag tag = event.GetData().GetTag();
  
  // If a subclass subscribed to the tag then let them handle it otherwise it is one we (the baseclass) subscribed
  // to, one of the kFailureTags
  if(_tagsSubclassSubscribeTo.count(tag) > 0)
  {
    HandleWhileRunningInternal(event, robot);
  }
  else
  {
    switch(tag)
    {
      case EngineToGameTag::RobotState:
      {
        break;
      }
      case EngineToGameTag::CliffEvent:
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
      }
      case EngineToGameTag::RobotStopped:
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
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
      }
      default:
      {
        return;
      }
    }
  }
}

void IBehaviorPlaypen::SubscribeToTags(std::set<EngineToGameTag>&& tags)
{
  _tagsSubclassSubscribeTo.insert(tags.begin(), tags.end());
  
  // If the caller wants to subscribe to one of the failure type tags then don't let them subscribe
  // since we already are subscribed to it and it will already call HandleWhileRunningInternal()
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
  
  IBehavior::SubscribeToTags(std::move(tags));
}

bool IBehaviorPlaypen::StartActing(IActionRunner* action, SimpleCallback callback)
{
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::ACTION_FAILED);
    }

    callback();
  };
  return IBehavior::StartActing(action, callbackWrapper);
}

bool IBehaviorPlaypen::StartActing(IActionRunner* action, ActionResultCallback callback)
{
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorPlaypen.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetIDStr().c_str(),
                          EnumToString(result));
    }
    
    callback(result);
  };
  return IBehavior::StartActing(action, callbackWrapper);
}

bool IBehaviorPlaypen::StartActing(IActionRunner* action, RobotCompletedActionCallback callback)
{
  auto callbackWrapper = [this, callback](const ExternalInterface::RobotCompletedAction& rca){
    if(rca.result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorPlaypen.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetIDStr().c_str(),
                          EnumToString(rca.result));
    }
    
    callback(rca);
  };
  return IBehavior::StartActing(action, callbackWrapper);
}

void IBehaviorPlaypen::WriteToStorage(Robot& robot, NVStorage::NVEntryTag tag,const u8* data, size_t size,
                                      FactoryTestResultCode failureCode)
{
  if(PlaypenConfig::kWriteToStorage)
  {
    PLAYPEN_TRY(robot.GetNVStorageComponent().Write(tag, data, size),
                failureCode);
  }
  else
  {
    PRINT_NAMED_INFO("IBehaviorPlaypen.WriteToStorage.WritingNotEnabled", "");
  }
}
  
bool IBehaviorPlaypen::ShouldIgnoreFailures() const
{
  return PlaypenConfig::kIgnoreFailures;
}

void IBehaviorPlaypen::SetResult(FactoryTestResultCode result)
{
  _result = result;
  _timers.clear();
  AddToResultList(result);
}

void IBehaviorPlaypen::AddToResultList(FactoryTestResultCode result)
{
  results[GetIDStr()].push_back(result);
}

const std::map<std::string, std::vector<FactoryTestResultCode>>& IBehaviorPlaypen::GetAllPlaypenResults()
{
  return results;
}
  
}
}
