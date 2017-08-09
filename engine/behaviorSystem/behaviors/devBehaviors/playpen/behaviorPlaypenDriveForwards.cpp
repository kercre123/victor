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
#include "engine/components/cliffSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const f32 kDistanceToTriggerFrontCliffs_mm = 90;
static const f32 kDistanceToTriggerBackCliffs_mm = 60;
static const f32 kDistanceToDriveOverCliff_mm = 30;
static const f32 kTimeToWaitForCliffEvent_ms = CLIFF_EVENT_DELAY_MS + 100;
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
  MoveHeadToAngleAction* headToZero = new MoveHeadToAngleAction(robot, 0);
  MoveLiftToHeightAction* liftDown = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
  DriveStraightAction* driveForwards = new DriveStraightAction(robot, kDistanceToTriggerFrontCliffs_mm);
  driveForwards->SetShouldPlayAnimation(false);
  CompoundActionParallel* action = new CompoundActionParallel(robot, {headToZero, liftDown, driveForwards});
  
  _waitingForCliffsState = WAITING_FOR_FRONT_CLIFFS;
  
  StartActing(action, [this, &robot](ActionResult result) {
    // Seeing the cliff will cause the drive straight action to be cancelled
    if(result == ActionResult::CANCELLED_WHILE_RUNNING)
    {
      // We have a delay between the action being cancelled from the RobotStopped event and
      // when the cliff event will be confirmed and sent from the robot so we need to wait a little bit
      // to receive it
      AddTimer(kTimeToWaitForCliffEvent_ms, [this, &robot]() {
        if(_frontCliffsDetected)
        {
          _waitingForCliffsState = WAITING_FOR_FRONT_CLIFFS_UNDETCTED;
          TransitionToWaitingForBackCliffs(robot);
        }
        else
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
        }
      });
    }
    else
    {
      // TODO: separate result for this case?
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
  _waitingForCliffsState = NONE;
  _frontCliffsDetected = false;
  _backCliffsDetected = false;
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForBackCliffs(Robot& robot)
{
  DriveStraightAction* action = new DriveStraightAction(robot, kDistanceToTriggerBackCliffs_mm);
  action->SetShouldPlayAnimation(false);
  
  StartActing(action, [this, &robot](ActionResult result) {
    // Seeing the cliff will cause the drive straight action to be cancelled
    if(result == ActionResult::CANCELLED_WHILE_RUNNING)
    {
      // We have a delay between the action being cancelled from the RobotStopped event and
      // when the cliff event will be confirmed and sent from the robot so we need to wait a little bit
      // to receive it
      AddTimer(kTimeToWaitForCliffEvent_ms, [this, &robot]() {
        if(_backCliffsDetected)
        {
          DEV_ASSERT(_frontCliffsDetected, "BehaviorPlaypenDriveForwards.FrontCliffsNotDetcted");
          _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS_UNDETECTED;
          TransitionToWaitingForBackCliffUndetected(robot);
        }
        else
        {
          PRINT_NAMED_WARNING("", "cliff event timer finished and no back cliffs detected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
        }
      });
    }
    else
    {
      // TODO: separate result for this case?
      PRINT_NAMED_WARNING("", "back cliff action not failed");
      PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
    }
  });
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForBackCliffUndetected(Robot& robot)
{
  DriveStraightAction* action = new DriveStraightAction(robot, kDistanceToDriveOverCliff_mm);
  action->SetShouldPlayAnimation(false);
  action->SetMotionProfile(DEFAULT_PATH_MOTION_PROFILE);
  
  StartActing(action, [this, &robot]() {
    AddTimer(kTimeToWaitForCliffEvent_ms, [this, &robot]() {
      if(_waitingForCliffsState == NONE)
      {
        const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
        GetLogger().AppendCliffValuesOnGround(cliffVals);
        
        PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
      }
      else
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_UNDETECTED);
      }
    });
  });
}

void BehaviorPlaypenDriveForwards::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  const EngineToGameTag tag = event.GetData().GetTag();
  if(tag == EngineToGameTag::CliffEvent)
  {
    const auto& payload = event.GetData().Get_CliffEvent();
    
    const bool cliffDetected = (payload.detectedFlags != 0);
    PRINT_NAMED_WARNING("", "Cliff event %d", cliffDetected);
    
    const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
    const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
    const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
    const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
    
    const bool frontCliffsDetected = (payload.detectedFlags & FR) && (payload.detectedFlags & FL);
    const bool backCliffsDetected = (payload.detectedFlags & BR) && (payload.detectedFlags & BL);

    switch(_waitingForCliffsState)
    {
      case NONE:
      {
        PRINT_NAMED_WARNING("", "got cliff event while in NONE state");
        // TODO: Need different failure
        PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
        break;
      }
      case WAITING_FOR_FRONT_CLIFFS:
      {
        if(cliffDetected)
        {
          if(frontCliffsDetected)
          {
            const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
            GetLogger().AppendCliffValuesOnFrontDrop(cliffVals);
            
            _frontCliffsDetected = true;
          }
          else
          {
            PRINT_NAMED_WARNING("", "waiting for front cliffs but not all front cliffs detected");
            PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
          }
        }
        else
        {
          PRINT_NAMED_WARNING("", "waiting for front cliffs but got cliff undetected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_UNDETECTED);
        }
        break;
      }
      case WAITING_FOR_FRONT_CLIFFS_UNDETCTED:
      {
        if(!cliffDetected)
        {
          PRINT_NAMED_WARNING("", "moving to waiting for back cliffs");
          _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS;
        }
        else
        {
          PRINT_NAMED_WARNING("", "waiting for front cliff undetected but detected cliff");
          // TODO: Need different failure
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
        }
        break;
      }
      case WAITING_FOR_BACK_CLIFFS:
      {
        if(cliffDetected)
        {
          if(backCliffsDetected)
          {
            const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                              robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
            GetLogger().AppendCliffValuesOnBackDrop(cliffVals);
            
            _backCliffsDetected = true;
          }
          else
          {
            PRINT_NAMED_WARNING("", "waiting for back cliffs but not all back cliffs detected");
            PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
          }
        }
        else
        {
          PRINT_NAMED_WARNING("", "got cliff undetected while waiting for back cliffs");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
        }
        break;
      }
      case WAITING_FOR_BACK_CLIFFS_UNDETECTED:
      {
        if(!cliffDetected)
        {
          _waitingForCliffsState = NONE;
        }
        else
        {
          PRINT_NAMED_WARNING("", "waiting for back cliff undetected but detected cliff");
          // TODO: Need different failure
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_UNDETECTED);
        }
        break;
      }
    }
  }
}

}
}


