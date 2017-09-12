/**
 * File: behaviorPlaypenDriveForwards.cpp
 *
 * Author: Al Chaussee
 * Created: 07/28/17
 *
 * Description: Drives Cozmo fowards over a narrow cliff checking that all 4 cliff sensors are working
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
  // Drive fowards some amount until front cliffs trigger, while cause the action to fail with
  // CANCELLED_WHILE_RUNNING
  MoveHeadToAngleAction* headToZero = new MoveHeadToAngleAction(robot, 0);
  MoveLiftToHeightAction* liftDown = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
  DriveStraightAction* driveForwards = new DriveStraightAction(robot,
                                                               PlaypenConfig::kDistanceToTriggerFrontCliffs_mm);
  driveForwards->SetShouldPlayAnimation(false);
  
  CompoundActionParallel* action = new CompoundActionParallel(robot, {headToZero, liftDown, driveForwards});
  
  _waitingForCliffsState = WAITING_FOR_FRONT_CLIFFS;
  
  StartActing(action, [this, &robot](ActionResult result) {
    // Seeing the cliff will cause the drive straight action to be cancelled
    // Action should only complete with CANCELLED_WHILE_RUNNING, if it completes in any other manner then
    // fail
    if(result != ActionResult::CANCELLED_WHILE_RUNNING)
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.InternalInitInternal.ActionCompleted",
                          "Expected DriveStraightAction to fail with %s but instead completed with %s",
                          EnumToString(ActionResult::CANCELLED_WHILE_RUNNING),
                          EnumToString(result));
      PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
    }
    
    // We have a delay between the action being cancelled from the RobotStopped event and
    // when the cliff event will be confirmed and sent from the robot so we need to wait a little bit
    // to receive it
    AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
      // If front cliff was successfully detected then move onto waiting for the back cliffs to be detected
      // otherwise fail
      if(!_frontCliffsDetected)
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
      }
      
      _waitingForCliffsState = WAITING_FOR_FRONT_CLIFFS_UNDETCTED;
      TransitionToWaitingForBackCliffs(robot);
    });
  });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenDriveForwards::InternalUpdateInternal(Robot& robot)
{
  // There are times during this behavior that we are not acting (waiting for a message to come from the robot)
  // so we need to keep Running using this empty InternalUpdateInternal
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
  // We have seen the front cliffs fire so we need to drive forwards until the back cliffs fire
  DriveStraightAction* action = new DriveStraightAction(robot, PlaypenConfig::kDistanceToTriggerBackCliffs_mm);
  action->SetShouldPlayAnimation(false);
  
  StartActing(action, [this, &robot](ActionResult result) {
    // Seeing the cliff will cause the drive straight action to be cancelled
    if(result != ActionResult::CANCELLED_WHILE_RUNNING)
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.TransitionToWaitingForBackCliffs.ActionCompleted",
                          "Expected DriveStraightAction to fail with %s but instead completed with %s",
                          EnumToString(ActionResult::CANCELLED_WHILE_RUNNING),
                          EnumToString(result));
      PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
    }
    
    // We have a delay between the action being cancelled from the RobotStopped event and
    // when the cliff event will be confirmed and sent from the robot so we need to wait a little bit
    // to receive it
    AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
      // If back cliffs were successfully detected then transition to waiting for them to be undetected
      // inidicating we are no longer on the cliff section of playpen
      if(!_backCliffsDetected)
      {
        PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.CliffEventTimeout.BackCliffsNotDetected", "");
        PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
      }
      
      // Front cliffs should have already been detected so fail if for some reason they haven't
      if(!_frontCliffsDetected)
      {
        PRINT_NAMED_ERROR("BehaviorPlaypenDriveForwards.CliffEventTimeout.FrontCliffsNotDetected",
                          "Back cliffs are detected but front cliffs are not, this shouldn't be possible");
        PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
      }
      
      _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS_UNDETECTED;
      
      TransitionToWaitingForBackCliffUndetected(robot);
      
    });
  });
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForBackCliffUndetected(Robot& robot)
{
  // Drive forwards completely over the cliff section of playpen
  DriveStraightAction* action = new DriveStraightAction(robot, PlaypenConfig::kDistanceToDriveOverCliff_mm);
  action->SetShouldPlayAnimation(false);
  action->SetMotionProfile(DEFAULT_PATH_MOTION_PROFILE);
  
  StartActing(action, [this, &robot]() {
    AddTimer(PlaypenConfig::kTimeToWaitForCliffEvent_ms, [this, &robot]() {
      // We should end up in the NONE state after successfully undetecting the back cliffs
      // We also expect the above DriveStraighAction to complete successfully in order to get here
      if(!(_waitingForCliffsState == NONE && _frontCliffsDetected && _backCliffsDetected))
      {
        PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.TransitionToWaitingForBackCliffUndetected.WrongState",
                            "Must not have seen back cliffs undetected in state %d",
                            _waitingForCliffsState);
        PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_UNDETECTED);
      }
      
      // All cliff sensors should be on the ground so record their values
      const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                        robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                        robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                        robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
      PLAYPEN_TRY(GetLogger().AppendCliffValuesOnGround(cliffVals), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
      
      PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
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
        // While waiting for front cliffs we expect to only get a cliff event that has seen a front cliff
        // otherwise we will fail
        if(!cliffDetected)
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.CliffUndetected",
                              "Waiting for front cliffs but got cliff undetected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNDETECTED);
        }
        
        if(!(frontCliffsDetected && !backCliffsDetected))
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.FrontCliffsNotDetected",
                              "Waiting for front cliffs but not all front cliffs detected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::FRONT_CLIFFS_NOT_DETECTED);
        }
        
        // Record all cliff sensor values when seeing just the front cliffs
        const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
        
        PLAYPEN_TRY(GetLogger().AppendCliffValuesOnFrontDrop(cliffVals),
                    FactoryTestResultCode::WRITE_TO_LOG_FAILED);
        
        PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForFrontCliffs.CliffDetected", "");
        
        _frontCliffsDetected = true;
        
        break;
      }
      case WAITING_FOR_FRONT_CLIFFS_UNDETCTED:
      {
        // If a cliff has been detected while waiting for cliffs to be undetected then fail
        if(cliffDetected)
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForFrontCliffsUndetected.CliffDetected",
                              "Waiting for front cliff undetected but detected cliff");
          
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
        }
        
        PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForFrontCliffsUndetected.CliffUndetected",
                         "Moving to waiting for back cliffs");
        
        _waitingForCliffsState = WAITING_FOR_BACK_CLIFFS;
        
        break;
      }
      case WAITING_FOR_BACK_CLIFFS:
      {
        if(!cliffDetected)
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffs.CliffUndetected",
                              "Got cliff undetected while waiting for back cliffs");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNDETECTED);
        }
        
        if(!(backCliffsDetected && !frontCliffsDetected))
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffs.BackCliffsNotDetected",
                               "Waiting for back cliffs but not all back cliffs detected");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_CLIFFS_NOT_DETECTED);
        }
        
        // Record all cliff sensor values when just seeing the back cliffs
        const CliffSensorValues cliffVals(robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_FL),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BR),
                                          robot.GetCliffSensorComponent().GetCliffDataRaw((u16)CliffSensor::CLIFF_BL));
        
        PLAYPEN_TRY(GetLogger().AppendCliffValuesOnBackDrop(cliffVals),
                    FactoryTestResultCode::WRITE_TO_LOG_FAILED);
        
        _backCliffsDetected = true;
        
        break;
      }
      case WAITING_FOR_BACK_CLIFFS_UNDETECTED:
      {
        if(cliffDetected)
        {
          PRINT_NAMED_WARNING("BehaviorPlaypenDriveForwards.WaitingForBackCliffsUndetected.CliffDetected",
                              "Waiting for back cliff undetected but detected cliff");
          PLAYPEN_SET_RESULT(FactoryTestResultCode::CLIFF_UNEXPECTED);
        }
        
        PRINT_NAMED_INFO("BehaviorPlaypenDriveForwards.WaitingForBackCliffsUndetected.CliffUndetected",
                         "Moving to state NONE");
        
        _waitingForCliffsState = NONE;
        
        break;
      }
    }
  }
}

}
}


