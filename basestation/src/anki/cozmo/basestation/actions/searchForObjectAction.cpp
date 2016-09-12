/**
 * File: searchForObjectAction
 *
 * Author: Mark Wesley
 * Created: 05/19/16
 *
 * Description: An action for Cozmo to look around to find either any cube or a specific cube
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/actions/searchForObjectAction.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
  
  
  SearchForObjectAction::SearchForObjectAction(Robot& robot, ObjectFamily desiredObjectFamily, ObjectID desiredObjectId, bool matchAnyObjectId)
    : IAction(robot,
              "SearchForObject",
              RobotActionType::SEARCH_FOR_OBJECT,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _compoundAction(robot)
    , _desiredObjectFamily(desiredObjectFamily)
    , _desiredObjectId(desiredObjectId)
    , _matchAnyObjectId(matchAnyObjectId)
    , _foundObject(false)
    , _shouldPopIdle(false)
  {
    std::string objectName = matchAnyObjectId ? "_Any" : ("_" + std::to_string(desiredObjectId));
    SetName(std::string("SearchForObject_") + EnumToString(desiredObjectFamily) + objectName);
    
    IExternalInterface* externalInterface = _robot.GetExternalInterface();
                     
    if (externalInterface != nullptr)
    {
      using namespace ExternalInterface;
      auto eventHandler = std::bind(&SearchForObjectAction::HandleEvent, this, std::placeholders::_1);
      _signalHandles.push_back( externalInterface->Subscribe(MessageEngineToGameTag::RobotObservedObject, eventHandler) );
    }
    
    PRINT_NAMED_INFO("SearchForObjectAction::SearchForObjectAction", "name = '%s'", GetName().c_str());
  }
  
  
  SearchForObjectAction::~SearchForObjectAction()
  {
    if (_shouldPopIdle)
    {
      _robot.GetAnimationStreamer().PopIdleAnimation();
      _shouldPopIdle = false;
    }
    _compoundAction.PrepForCompletion();
  }

  
  ActionResult SearchForObjectAction::Init()
  {
    // Incase we are re-running this action
    _compoundAction.ClearActions();
    _compoundAction.EnableMessageDisplay(IsMessageDisplayEnabled());
    
    // Move head to look straight forward
    {
      MoveHeadToAngleAction* moveHead = new MoveHeadToAngleAction(_robot, kIdealViewBlockHeadAngle);
      _compoundAction.AddAction(moveHead);
    }

    // Turns of >Pi aren't supported, also turning at any real speed makes Cozmo fail to see markers
    // so split into multiple smaller turns - turning fast for a small angle, and then pause for long enough to
    // see any markers we missed, and then turn fast again
    
    const float turnDir = (GetRNG().RandDbl() > 0.5) ? 1.0f : -1.0f;
    const float kTotalAngleToTurn = 2.0f * M_PI;
    const float kSpeedForFastTurn = 1.0f;
    const float kMinAngleForFastTurn = DEG_TO_RAD_F32(15.f);
    const float kMaxAngleForFastTurn = DEG_TO_RAD_F32(25.f);
    const float kMinWaitTime_s = 0.4f;
    const float kMaxWaitTime_s = 0.75f;

    float remainingAngleToTurn = kTotalAngleToTurn;
    bool isFastTurn = true;
    uint32_t numTurns = 0;
    
    while (Util::IsFltGT(remainingAngleToTurn, 0.0f))
    {
      if (isFastTurn)
      {
        const float desiredAngleToTurn = GetRNG().RandDblInRange(kMinAngleForFastTurn, kMaxAngleForFastTurn);
        const float angleToTurn = Util::Min(remainingAngleToTurn, desiredAngleToTurn);
        remainingAngleToTurn -= angleToTurn;
        const float speedToTurn = kSpeedForFastTurn;

        TurnInPlaceAction* newTurn = new TurnInPlaceAction(_robot, (angleToTurn * turnDir), false);
        newTurn->SetMaxSpeed(speedToTurn);
        newTurn->SetAccel(2.0f);
        newTurn->SetTolerance(DEG_TO_RAD(4.0f));
        _compoundAction.AddAction(newTurn);
        ++numTurns;
        
        //PRINT_NAMED_INFO("SearchForObjectAction.Init", "Adding %.1f (%.1f d) Turn, %.1f (%.1f deg) left",
        //                 angleToTurn, RAD_TO_DEG_F32(angleToTurn), remainingAngleToTurn, RAD_TO_DEG_F32(remainingAngleToTurn));
      }
      else
      {
        const float waitTime_s = GetRNG().RandDblInRange(kMinWaitTime_s, kMaxWaitTime_s);
        WaitAction* newWaitAction = new WaitAction(_robot, waitTime_s);
        _compoundAction.AddAction(newWaitAction);
        
        //PRINT_NAMED_INFO("SearchForObjectAction.Init", "Waiting %.1f seconds", waitTime_s);
      }
      
      isFastTurn = !isFastTurn;
    }
    
    PRINT_NAMED_INFO("SearchForObjectAction.Init", "Will do %u turn-and-wait combos (dir %.1f)", numTurns, turnDir);
    
    // Prevent the compound action from signaling completion
    _compoundAction.ShouldEmitCompletionSignal(false);
    
    // Prevent the compound action from locking tracks (SearchForObjectAction locks them already)
    _compoundAction.ShouldSuppressTrackLocking(true);
    
    // disable the live idle animation, so we aren't moving during the "wait" sections
    if( !_shouldPopIdle )
    {
      _shouldPopIdle = true;
      _robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
    }
    
    return ActionResult::SUCCESS;
  }
  
  
  ActionResult SearchForObjectAction::CheckIfDone()
  {
    if (_foundObject)
    {
      return ActionResult::SUCCESS;
    }
    
    ActionResult compoundActionResult = _compoundAction.Update();
    if (compoundActionResult == ActionResult::SUCCESS)
    {
      // turn finished, but we didn't find the object
      return ActionResult::FAILURE_PROCEED;
    }

    return compoundActionResult;
  }
  

  void SearchForObjectAction::HandleEvent(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
  {
    const ExternalInterface::MessageEngineToGame& outerMsg = event.GetData();
    const ExternalInterface::MessageEngineToGame::Tag messageTag = outerMsg.GetTag();
    switch (messageTag)
    {
      case ExternalInterface::MessageEngineToGameTag::RobotObservedObject:
      {
        const ExternalInterface::RobotObservedObject& msg = outerMsg.Get_RobotObservedObject();
        
        if (msg.objectFamily == _desiredObjectFamily)
        {
          if (_matchAnyObjectId || (msg.objectID == _desiredObjectId))
          {
            _foundObject = true;
            PRINT_NAMED_INFO("SearchForObjectAction.RobotObserved.Match", "Robot %u at time %u %s:%s id %d",
                             msg.robotID, msg.timestamp, EnumToString(msg.objectFamily), EnumToString(msg.objectType), msg.objectID);
          }
        }
        
        break;
      }
      default:
      {
        PRINT_NAMED_ERROR("SearchForObjectAction.HandleEvent.UnhandledType",
                          "Unhandled Message Type %d '%s'", (int)messageTag, MessageEngineToGameTagToString(messageTag) );
        assert(0);
        break;
      }
    }
  }
    
  
} // namespace Cozmo
} // namespace Anki
