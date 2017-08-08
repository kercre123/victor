/**
 * File: behaviorPlaypenDriveForwards.cpp
 *
 * Author: Al Chaussee
 * Created: 07/28/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDriveForwards.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/blockWorld/blockWorld.h"
//#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const f32 kDistanceToTriggerFrontCliffs_mm = 90;
static const f32 kDistanceToTriggerBackCliffs_mm = 60;
}

BehaviorPlaypenDriveForwards::BehaviorPlaypenDriveForwards(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({{EngineToGameTag::RobotStopped,
                    EngineToGameTag::CliffEvent}});
}

void BehaviorPlaypenDriveForwards::GetResultsInternal()
{
  
}

Result BehaviorPlaypenDriveForwards::InternalInitInternal(Robot& robot)
{
  // Prevent robot from stopping on cliffs
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));

  MoveHeadToAngleAction* headToZero = new MoveHeadToAngleAction(robot, 0);
  MoveLiftToHeightAction* liftDown = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
  DriveStraightAction* driveForwards = new DriveStraightAction(robot, kDistanceToTriggerFrontCliffs_mm);
  driveForwards->SetShouldPlayAnimation(false);
  CompoundActionParallel* action = new CompoundActionParallel(robot, {headToZero, liftDown, driveForwards});
  
  _waitingForFrontCliffs = true;
  
  StartActing(action, [this, &robot](){
    if(_frontCliffsDetected)
    {
      _waitingForFrontCliffs = false;
      TransitionToWaitingForBackCliffs(robot);
    }
    else
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
    }
  });
  
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenDriveForwards::InternalUpdateInternal(Robot& robot)
{
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenDriveForwards::StopInternal(Robot& robot)
{
  _waitingForFrontCliffs = false;
  _frontCliffsDetected = false;
  _backCliffsDetected = false;
  
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(true)));
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForBackCliffs(Robot& robot)
{
  DriveStraightAction* action = new DriveStraightAction(robot, kDistanceToTriggerBackCliffs_mm);
  action->SetShouldPlayAnimation(false);
  
  StartActing(action, [this, &robot](){
    if(_backCliffsDetected)
    {
      DEV_ASSERT(_frontCliffsDetected, "BehaviorPlaypenDriveForwards.FrontCliffsNotDetcted");
      PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
    }
    else
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
    }
  });
}

void BehaviorPlaypenDriveForwards::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  const EngineToGameTag tag = event.GetData().GetTag();
  if(tag == EngineToGameTag::CliffEvent)
  {
    const auto& payload = event.GetData().Get_CliffEvent();
    
    const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
    const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
    const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
    const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
    
    const bool frontCliffsDetected = (payload.detectedFlags & FR) && (payload.detectedFlags & FL);
    const bool backCliffsDetected = (payload.detectedFlags & BR) && (payload.detectedFlags & BL);
    
    if(_waitingForFrontCliffs)
    {
      if(frontCliffsDetected)
      {
        _frontCliffsDetected = true;
      }
      else
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
      }
    }
    else
    {
      if(backCliffsDetected)
      {
        _backCliffsDetected = true;
      }
      else
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
      }
    }
  }
}

}
}


