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

BehaviorPlaypenDriveForwards::BehaviorPlaypenDriveForwards(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({{EngineToGameTag::RobotStopped,
                    EngineToGameTag::CliffEvent}});
}

Result BehaviorPlaypenDriveForwards::InternalInitInternal(Robot& robot)
{
  MoveHeadToAngleAction* headToZero = new MoveHeadToAngleAction(robot, 0);
  MoveLiftToHeightAction* liftDown = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
  DriveStraightAction* driveForwards = new DriveStraightAction(robot,
                                                               PlaypenConfig::kDistanceToTriggerFrontCliffs_mm);
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
      AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
        if(_frontCliffsDetected)
        {
          _waitingForCliffsState = WAITING_FOR_FRONT_CLIFFS_UNDETCTED;
          TransitionToWaitingForBackCliffs(robot);
        }
        else
        {
          PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
        }
      });
    }
    else
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.InternalInitInternal.ActionCompleted",
                          "Expected DriveStraightAction to fail with %s but instead completed with %s",
                          EnumToString(ActionResult::CANCELLED_WHILE_RUNNING),
                          EnumToString(result));
      PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
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
  DriveStraightAction* action = new DriveStraightAction(robot, PlaypenConfig::kDistanceToTriggerBackCliffs_mm);
  action->SetShouldPlayAnimation(false);
  
  StartActing(action, [this, &robot](ActionResult result) {
    // Seeing the cliff will cause the drive straight action to be cancelled
    if(result == ActionResult::CANCELLED_WHILE_RUNNING)
    {
      // We have a delay between the action being cancelled from the RobotStopped event and
      // when the cliff event will be confirmed and sent from the robot so we need to wait a little bit
      // to receive it
      AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
        if(_backCliffsDetected)
        {
          if(!_frontCliffsDetected)
          {
            PRINT_NAMED_ERROR("BehaviorPlaypenDriveForwards.CliffEventTimeout.FrontCliffsNotDetected",
                              "Back cliffs are detected but front cliffs are not, this shouldn't be possible");
            PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
          }
          
          _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS_UNDETECTED;
          TransitionToWaitingForBackCliffUndetected(robot);
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.CliffEventTimeout.BackCliffsNotDetected", "");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
        }
      });
    }
    else
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.TransitionToWaitingForBackCliffs.ActionCompleted",
                          "Expected DriveStraightAction to fail with %s but instead completed with %s",
                          EnumToString(ActionResult::CANCELLED_WHILE_RUNNING),
                          EnumToString(result));
      PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
    }
  });
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForBackCliffUndetected(Robot& robot)
{
  DriveStraightAction* action = new DriveStraightAction(robot, PlaypenConfig::kDistanceToDriveOverCliff_mm);
  action->SetShouldPlayAnimation(false);
  action->SetMotionProfile(DEFAULT_PATH_MOTION_PROFILE);
  
  StartActing(action, [this, &robot]() {
    AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
      if(_waitingForCliffsState == NONE)
      {
        const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
        PLAYPEN_TRY(GetLogger().AppendCliffValuesOnGround(cliffVals), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
        
        PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.TransitionToWaitingForBackCliffUndetected.WrongState",
                            "Must not have seen back cliffs undetected in state %d",
                            _waitingForCliffsState);
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
    
    PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.CliffEvent",
                     "CliffDetected?: %d, FR: %u, FL: %u, BR: %u, BL: %u",
                     cliffDetected,
                     robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                     robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                     robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                     robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
    
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
        PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.UnexpectedCliffEvent",
                            "Got cliff event while in NONE state");
        PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
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
            PLAYPEN_TRY(GetLogger().AppendCliffValuesOnFrontDrop(cliffVals),
                        FactoryTestResultCode::WRITE_TO_LOG_FAILED);
            
            PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.CliffDetected", "");
            
            _frontCliffsDetected = true;
          }
          else
          {
            PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.FrontCliffsNotDetected",
                                "Waiting for front cliffs but not all front cliffs detected");
            PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
          }
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.CliffUndetected",
                              "Waiting for front cliffs but got cliff undetected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNDETECTED);
        }
        break;
      }
      case WAITING_FOR_FRONT_CLIFFS_UNDETCTED:
      {
        if(!cliffDetected)
        {
          PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForFrontCliffsUndetected.CliffUndetected",
                           "Moving to waiting for back cliffs");
          
          _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS;
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffsUndetected.CliffDetected",
                              "Waiting for front cliff undetected but detected cliff");
          // TODO: Need different failure
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
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
            PLAYPEN_TRY(GetLogger().AppendCliffValuesOnBackDrop(cliffVals),
                        FactoryTestResultCode::WRITE_TO_LOG_FAILED);
            
            _backCliffsDetected = true;
          }
          else
          {
            PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffs.BackCliffsNotDetected",
                                "Waiting for back cliffs but not all back cliffs detected");
            PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
          }
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffs.CliffUndetected",
                              "Got cliff undetected while waiting for back cliffs");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNDETECTED);
        }
        break;
      }
      case WAITING_FOR_BACK_CLIFFS_UNDETECTED:
      {
        if(!cliffDetected)
        {
          PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForBackCliffsUndetected.CliffUndetected",
                           "Moving to state NONE");
          
          _waitingForCliffsState = NONE;
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffsUndetected.CliffDetected",
                              "Waiting for back cliff undetected but detected cliff");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
        }
        break;
      }
    }
  }
}

}
}


