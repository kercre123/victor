/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqPlaypen.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kIgnoreFailures,    "Playpen", true);
CONSOLE_VAR(bool, kWriteToStorage,    "Playpen", false);
CONSOLE_VAR(f32,  kDefaultTimeout_ms, "Playpen", 20000);
 
IBehaviorPlaypen::IBehaviorPlaypen(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  IBehavior::SubscribeToTags({{EngineToGameTag::RobotState,
                               EngineToGameTag::CliffEvent,
                               EngineToGameTag::RobotStopped,
                               EngineToGameTag::ChargerEvent,
                               EngineToGameTag::MotorCalibration,
                               EngineToGameTag::MotorAutoEnabled,
                               EngineToGameTag::RobotOffTreadsStateChanged,
                               EngineToGameTag::UnexpectedMovement}});
}

bool IBehaviorPlaypen::IsRunnableInternal(const BehaviorPreReqPlaypen& preReq) const
{
  _factoryTestLogger = &preReq.GetFactoryTestLogger();
  return true;
}

Result IBehaviorPlaypen::InitInternal(Robot& robot)
{
  AddTimer(kDefaultTimeout_ms, [this](){
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

  for(auto& timer : _timers)
  {
    timer.Tick();
  }
  
  return InternalUpdateInternal(robot);
}
  
void IBehaviorPlaypen::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  EngineToGameTag tag = event.GetData().GetTag();
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

void IBehaviorPlaypen::WriteToStorage(Robot& robot, NVStorage::NVEntryTag tag,const u8* data, size_t size)
{
  if(kWriteToStorage)
  {
    PLAYPEN_TRY(robot.GetNVStorageComponent().Write(tag, data, size),
                FactoryTestResultCode::NVSTORAGE_WRITE_FAILED);
  }
  else
  {
    PRINT_NAMED_INFO("IBehaviorPlaypen.WriteToStorage.WritingNotEnabled", "");
  }
}
  
bool IBehaviorPlaypen::ShouldIgnoreFailures() const {
  return kIgnoreFailures;
}
  
}
}
