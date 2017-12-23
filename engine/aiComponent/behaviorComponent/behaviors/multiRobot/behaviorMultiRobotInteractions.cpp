/**
 * File: BehaviorMultiRobotInteractions.cpp
 *
 * Author: Kevin Yoon
 * Date:   12/18/2017
 *
 * Description: Kevin's R&D behavior for demonstrating interactions
 *              between Victors
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/multiRobot/behaviorMultiRobotInteractions.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/jsonTools.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/stateConceptStrategyFactory.h"

#include "engine/components/multiRobotComponent.h"

#include "clad/types/objectTypes.h"

#include <vector>

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

namespace{
  const ObjectType kMultiRobotLandmark = ObjectType::Block_LIGHTCUBE1;  // Add to config

  RobotInteraction _requestedInteraction = RobotInteraction::Invalid;
  bool _requestPending = false;
  bool _isMaster = false;

  TimeStamp_t _lastTimeHeadMoved = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorMultiRobotInteractions::BehaviorMultiRobotInteractions(const Json::Value& config)
: ICozmoBehavior(config)
, _multiRobotComponent(nullptr)
, _state(State::Idle)
{

  SubscribeToTags({
    EngineToGameTag::MultiRobotSessionStarted,
  });

  // const Json::Value& configArray = config[kGestureToAnimationKey];
  // assert(configArray.isArray());

  // for( const auto& triggerConfig : configArray ) {
  //   auto anim = JsonTools::ParseString(triggerConfig, kAnimationNameKey, "Failed to parse animation name");
  //   auto rate = JsonTools::ParseFloat(triggerConfig, kAnimationRateKey, "Failed to parse animation rate");

  //   _tgAnimConfigs.emplace_back(StateConceptStrategyFactory::CreateStateConceptStrategy(triggerConfig),
  //                               anim,
  //                               rate,
  //                               0.0f);
  // }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorMultiRobotInteractions::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotInteractions::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(MultiRobotFistBumpMaster),
                                 BEHAVIOR_CLASS(MultiRobotFistBump),
                                 _fistBumpMasterBehavior);

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(MultiRobotFistBumpSlave),
                                 BEHAVIOR_CLASS(MultiRobotFistBump),
                                 _fistBumpSlaveBehavior);                                 


  if (behaviorExternalInterface.HasMultiRobotComponent()) {
    _multiRobotComponent = &behaviorExternalInterface.GetMultiRobotComponent();
    _multiRobotComponent->SetLandmark(kMultiRobotLandmark);
    PRINT_NAMED_INFO("BehaviorMultiRobotInteractions.InitBehvaior.Start", "");
  } else {
    PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.InitBehvaior.NoMultiRobotComponent", "");
    return;
  }  

}

void BehaviorMultiRobotInteractions::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_fistBumpMasterBehavior.get());
  delegates.insert(_fistBumpSlaveBehavior.get());  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorMultiRobotInteractions::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if (_multiRobotComponent) {
    return RESULT_OK;
  }
    
  PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.OnBehaviorActivated.NullMultiRobotComponent", "");
  return RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus BehaviorMultiRobotInteractions::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto now_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  bool isPickedUp = behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads;

  switch(_state) {
    case State::Idle:
    {
      SET_STATE(WaitForPickup);
      break;
    }
    case State::WaitForPickup:
    {
      if (isPickedUp) {
        SET_STATE(WaitForPutdown);
      }
      break;
    }
    case State::WaitForPutdown:
    {
      if (!isPickedUp && !_multiRobotComponent->IsInSession()) {
        SET_STATE(RequestInteraction);
      }
      break;
    }
    case State::RequestInteraction:
    {
      auto robotList = _multiRobotComponent->GetRobotsLocatedToLandmark();
      if (!robotList.empty()) {

        // TODO: Randomly choose
        _requestedInteraction = RobotInteraction::FistBump;

        RobotID_t requestedRobotID = robotList[0];
        PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.Update.RequestInteraction", "RobotID: %d", requestedRobotID);

        MultiRobotComponent::RequestInteractionCallback requestCallback = [this](bool accepted) {
          PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.Update.RequestCallback", 
                              "Accepted: %d, InSession: %d", accepted, _multiRobotComponent->IsInSession());
          if (!accepted && !_multiRobotComponent->IsInSession()) {
            SET_STATE(Idle);
          }
        };

        _multiRobotComponent->RequestInteraction(requestedRobotID, _requestedInteraction, requestCallback);
        _requestPending = true;
        SET_STATE(WaitForResponse);
      }
      
      break;
    }
    case State::WaitForResponse:
    {
      if (!_requestPending) {
        if(_requestedInteraction == RobotInteraction::Invalid) {
          // The session was rejected
          SET_STATE(Idle);
        } else {
          
          auto beh = _isMaster ? _fistBumpMasterBehavior : _fistBumpSlaveBehavior;

          if( beh->WantsToBeActivated(behaviorExternalInterface) ) {
            PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.Update.StartingInteraction", "%s (%d)", EnumToString(_requestedInteraction), _isMaster);
            DelegateIfInControl( behaviorExternalInterface,
                                beh.get(),
                                [](BehaviorExternalInterface& behaviorExternalInterface) {
                                  PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.Update.FistBumpComplete", "");
                                  _requestedInteraction = RobotInteraction::Invalid;
                                } );

            SET_STATE(DoInteraction);
          }
        }

      } 
      break;
    }
    case State::DoInteraction:
    {
      if (_requestedInteraction == RobotInteraction::Invalid) {
        PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.DoInteraction.BehaviorComplete", "");
        _multiRobotComponent->TerminateSession();

        CompoundActionParallel* action = new CompoundActionParallel();
        action->AddAction(new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK));
        action->AddAction(new MoveHeadToAngleAction(0));
        DelegateIfInControl(action);
        SET_STATE(Idle);
      }
      break;
    }
  }

  // TODO: Stop robot for a second for poseWrtLandmark to settle
  // ...
  
  //PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.Update", "%d", _state);

  if ((_requestedInteraction == RobotInteraction::Invalid) && 
      !isPickedUp &&
      (now_ms - _lastTimeHeadMoved > 5000)) {
    DelegateIfInControl(new MoveHeadToAngleAction(0));
    _lastTimeHeadMoved = now_ms;
  }

  return BehaviorStatus::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotInteractions::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotInteractions::HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::MultiRobotSessionStarted:
      HandleMultiRobotSessionStarted(event.GetData().Get_MultiRobotSessionStarted(), behaviorExternalInterface);
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorMultiRobotInteractions.HandleWhileActivated.InvalidEvent", "");
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorMultiRobotInteractions::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}
  

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// TODO: Should these just be callbacks on MultiRobotComponent instead?
//       This seems risky in that there's no clear session owner.
void BehaviorMultiRobotInteractions::HandleMultiRobotSessionStarted(const ExternalInterface::MultiRobotSessionStarted& msg, BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_NAMED_WARNING("BehaviorMultiRobotInteractions.MultiRobotSessionStarted", "");

  _requestPending = false;
  _requestedInteraction = msg.interaction;
  _isMaster = msg.isMaster;
  SET_STATE(WaitForResponse);

  // if( _fistBumpBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
  //   DelegateIfInControl( behaviorExternalInterface,
  //                        _fistBumpBehavior.get() );
  // }
}

} // Cozmo
} // Anki
