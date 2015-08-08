/**
 * File: behaviorLookForFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "LookForFaces" behavior, which searches for people's
 *              faces and tracks/interacts with them if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Cozmo {

  BehaviorLookForFaces::BehaviorLookForFaces(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::LOOKING_AROUND)
  {
    
    
  }
  
  Result BehaviorLookForFaces::Init()
  {
    _currentState = State::LOOKING_AROUND;
    
    // Set up signal handlers for while we are running
    
    //TODO: Remove once Lee's Events are in
    auto cbObservedObject = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) {
      this->HandleRobotObservedObject(event.GetData().Get_RobotObservedObject());
    };
    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedObject, cbObservedObject));
    
    auto cbActionCompleted = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) {
      this->HandleRobotCompletedAction(event.GetData().Get_RobotCompletedAction());
    };
    _eventHandles.emplace_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotCompletedAction, cbActionCompleted));
    
    return RESULT_OK;
  }
  
  
  IBehavior::Status BehaviorLookForFaces::Update(float currentTime_sec)
  {
    switch(_currentState)
    {
      case State::LOOKING_AROUND:
        
        if(currentTime_sec - _lastLookAround_sec > _nextMovementTime_sec)
        {
          // Time to move again: tilt head and move body by a random amount
          // TODO: Get the angle limits from config
          const f32 headAngle_deg = _rng.RandIntInRange(0, MAX_HEAD_ANGLE);
          const f32 bodyAngle_deg = _rng.RandIntInRange(-90, 90);
          
          MoveHeadToAngleAction* moveHeadAction    = new MoveHeadToAngleAction(DEG_TO_RAD(headAngle_deg));
          TurnInPlaceAction*     turnInPlaceAction = new TurnInPlaceAction(DEG_TO_RAD(bodyAngle_deg));
          
          _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot,
                                                new CompoundActionParallel({moveHeadAction, turnInPlaceAction}));
        }

        break;
        
      case State::TRACKING_FACE:
        if(_lastSeen_ms - _firstSeen_ms > SEC_TO_MILIS(_trackingTimeout_sec)) {
          _robot.DisableTrackToObject();
          _currentState = State::LOOKING_AROUND;
          SetNextMovementTime();
        }
        break;
        
      case State::INTERRUPTED:
        // This is the only way we stop this behavior: unsubscribe to all signals.
        _eventHandles.clear();
        return Status::COMPLETE;
        
      default:
        
        return Status::FAILURE;
    } // switch(_currentState)
    
    return Status::RUNNING;
  } // Update()
  
  Result BehaviorLookForFaces::Interrupt()
  {
    _robot.DisableTrackToObject();
    _currentState = State::INTERRUPTED;
    
    return RESULT_OK;
  }
  
  bool BehaviorLookForFaces::GetRewardBid(Reward& reward)
  {
    // TODO: Fill reward  in...
    return true;
  }
  
#pragma mark -
#pragma mark Signal Handlers
  
  void BehaviorLookForFaces::HandleRobotObservedObject(const ExternalInterface::RobotObservedObject &msg)
  {
    // we only care about observed faces
    if(msg.objectFamily == BlockWorld::ObjectFamily::HUMAN_HEADS)
    {
      ObjectID objectID;
      objectID = msg.objectID;
      
      switch(_currentState)
      {
        case State::LOOKING_AROUND:
        {
          Vision::ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(objectID);
          if(object == nullptr) {
            PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotObservedObject.InvalidID",
                              "Could not get object %d.\n", msg.objectID);
            return;
          }
          
          // If we aren't already tracking, start tracking the next face we see
          _firstSeen_ms = _lastSeen_ms = object->GetLastObservedTime();
          _robot.EnableTrackToObject(objectID, false);
          
          break;
        }
          
        case State::TRACKING_FACE:
          if(objectID == _robot.GetTrackToObject()) {
            Vision::ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(objectID);
            if(object == nullptr) {
              PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotObservedObject.InvalidID",
                                "Could not get object %d.\n", msg.objectID);
              return;
            }
            _lastSeen_ms = object->GetLastObservedTime();
            
            //
            // TODO: Get head pose / expression and mimic
            //
            
          }
          break;
          
        default:
          PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotObservedObject.UnknownState",
                            "Reached state = %d\n", _currentState);
      }
    }
  }
  
  void BehaviorLookForFaces::SetNextMovementTime()
  {
    // TODO: Get the timing variability from config
    const f32 minTime_sec = 0.2f;
    const f32 maxTime_sec = 1.5f;
    _nextMovementTime_sec = _rng.RandDblInRange(minTime_sec, maxTime_sec);
  }
  
  void BehaviorLookForFaces::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
  {
    if(msg.actionType == RobotActionType::COMPOUND)
    {
      // Last movement finished: set next time to move
      SetNextMovementTime();
    }
  }
} // namespace Cozmo
} // namespace Anki