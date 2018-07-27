/**
 * File: onboardingStageCube.h
 *
 * Author: ross
 * Created: 2018-06-07
 *
 * Description: Onboarding logic unit for cube, which finds a cube, sees it, "activates" it, and picks it up
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingActivateCube.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "proto/external_interface/messages.pb.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const unsigned int kMaxActivationAttempts = 2;
  const unsigned int kMaxPickUpAttempts = 3;
  const float kMaxSearchDuration_s = 30.0f;
  const int kMaxAgeForRedoSearch_ms = 5000;
  
  const std::string& kLockName = "onboardingCubeStage";
}

class OnboardingStageCube : public IOnboardingStage
{
public:
  OnboardingStageCube()
  {
    Json::Value conditionConfig = IBEICondition::GenerateBaseConditionConfig( BEIConditionType::ObjectKnown );
    conditionConfig["objectTypes"].append("Block_LIGHTCUBE1");
    conditionConfig["objectTypes"].append("Block_LIGHTCUBE2");
    conditionConfig["objectTypes"].append("Block_LIGHTCUBE3");
    conditionConfig["maxAge_ms"] = 0; // only look for cubes sighted this tick
    _cubeKnownCondition.reset( new ConditionObjectKnown(conditionConfig) );
    _cubeKnownCondition->SetOwnerDebugLabel( "BehaviorOnboarding" ); // this probably won't work right with webviz
  }
  virtual ~OnboardingStageCube() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    delegates.insert( BEHAVIOR_ID(OnboardingLookAtUser) );
    delegates.insert( BEHAVIOR_ID(OnboardingLookForCube) );
    delegates.insert( BEHAVIOR_ID(OnboardingActivateCube) );
    delegates.insert( BEHAVIOR_ID(OnboardingPickUpCube) );
    // todo: different celebration than what PickUp does
    // design says "super stoked then calms down"
    delegates.insert( BEHAVIOR_ID(PutDownBlock) );
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _selectedBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    // load all behaviors
    _behaviors.clear();
    
    _behaviors[Step::LookingAtUser]    = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::LookingForCube]   = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookForCube) );
    _behaviors[Step::ActivatingCube]   = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingActivateCube) );
    _behaviors[Step::PickingUpCube]    = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingPickUpCube) );
    _behaviors[Step::PuttingCubeDown]  = GetBehaviorByID( bei, BEHAVIOR_ID(PutDownBlock) );
    
    bei.GetBehaviorContainer().FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingPickUpCube),
                                                            BEHAVIOR_CLASS(PickUpCube),
                                                            _pickUpBehavior );
    bei.GetBehaviorContainer().FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingActivateCube),
                                                            BEHAVIOR_CLASS(OnboardingActivateCube),
                                                            _activationBehavior );
    _putDownBehavior = bei.GetBehaviorContainer().FindBehaviorByID( BEHAVIOR_ID(PutDownBlock) );
    
    // initial behavior
    TransitionToLookingAtUser();
    
    // disable trigger word
    SetTriggerWordEnabled(false);
    
    _cubeKnownCondition->Init( bei );
    
    _activationAttemptCount = 0;
    _pickUpAttemptCount = 0;
    _timeStartedSearching_ms = 0;
  }
  
  virtual bool OnContinue( BehaviorExternalInterface& bei, int stepNum ) override
  {
    bool accepted = false;
    if( _step == Step::LookingAtUser ) {
      accepted = (stepNum == external_interface::STEP_EXPECTING_START_CUBE_SEARCH);
      if( accepted ) {
        TransitionToLookingForCube( bei );
      }
    } else if( _step != Step::Complete ) {
      DEV_ASSERT(false, "OnboardingStageCube.UnexpectedContinue");
    }
    return accepted;
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    DebugTransition( "Skipped." );
    TransitionToComplete( bei );
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::ActivatingCube ) {
      ++_activationAttemptCount;
      if( _activationBehavior->WasSuccessful() ) {
        TransitionToPickingUpCube( bei );
      } else {
        if( _activationAttemptCount <= kMaxActivationAttempts ) {
          TransitionToLookingForCube( bei );
        } else {
          // couldn't see the marker enough to "activate" it after a couple attempts.
          // this shouldn't happen since we remove the requirement for the behavior to
          // visually verify the cube, but just in case, move on
          PRINT_NAMED_WARNING( "OnboardingStageCube.OnBehaviorDeactivated.NoActivation",
                               "Cube activation repeatedly failed" );
          TransitionToPickingUpCube( bei );
          
        }
      }
    } else if( _step == Step::PickingUpCube ) {
      ++_pickUpAttemptCount;
      const bool isCarrying = bei.GetRobotInfo().IsCarryingObject();
      if( !isCarrying && (_pickUpAttemptCount <= kMaxPickUpAttempts) ) {
        if( HasCubeBeenSeenRecently( bei ) ) {
          // try using this cube again
          TransitionToPickingUpCube( bei );
        } else {
          TransitionToLookingForCube( bei );
        }
      } else {
        // success! or silent (to the app) failure!
        if( !isCarrying ) {
          DebugTransition( "Pickup failed too many times. Complete." );
        } else {
          DebugTransition( "Pickup Success." );
        }
        TransitionToPuttingDownCube( bei );
      }
    } else if( _step == Step::PuttingCubeDown ) {
      TransitionToComplete( bei );
    }
    
    
  }
  
  virtual bool OnInterrupted( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    auto& drivingAnimHandler = bei.GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.RemoveDrivingAnimations( kLockName );
    
    // interruptions at any step do not end this stage
    return false;
  }
  
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    // these steps may need to do something upon resuming. other steps are already saved in _selectedBehavior
    if( _step == Step::ActivatingCube ) {
      if( !HasCubeBeenSeenRecently( bei ) ) {
        TransitionToLookingForCube( bei );
      } else {
        TransitionToActivatingCube( bei );
      }
    } else if( _step == Step::PickingUpCube ) {
      if( !HasCubeBeenSeenRecently( bei ) ) {
        TransitionToLookingForCube( bei );
      } else {
        TransitionToPickingUpCube( bei );
      }
    } else if( _step == Step::PuttingCubeDown ) {
      TransitionToPuttingDownCube( bei );
    }
    
  }
  
  virtual void Update( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::LookingForCube ) {
      // a lot of this logic will change once cube connection is sorted out. For now, we just use the first
      // cube we see, and play lights on whatever cube we're connected to, if any.
      // also: things like if an active cube moves etc etc
      const bool seesCube = _cubeKnownCondition->AreConditionsMet(bei);
      TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if( seesCube ) {
        const auto& cubes = _cubeKnownCondition->GetObjects( bei );
        if( !cubes.empty() ) {
          DebugTransition("Found a cube");
          // pick one for now. it will probably be a seen cube + connected cube, or just a seen cube if we can't connect
          const auto* cube = cubes.front();
          _objectID = cube->GetID();
          TransitionToActivatingCube( bei );
        }
      } else if( currTime_ms >= _timeStartedSearching_ms + 1000*kMaxSearchDuration_s ) {
        auto* gi = bei.GetRobotInfo().GetGatewayInterface();
        if( gi != nullptr ){
          DebugTransition("Can't find a cube");
          auto* cantFindCubeMsg = new external_interface::OnboardingCantFindCube;
          gi->Broadcast( ExternalMessageRouter::Wrap(cantFindCubeMsg) );
          TransitionToLookingAtUser();
        }
      }
    }
  }
  
  virtual int GetExpectedStep() const override
  {
    switch( _step ) {
      case Step::LookingAtUser:
        return external_interface::STEP_EXPECTING_START_CUBE_SEARCH;
      case Step::LookingForCube:
        return external_interface::STEP_CUBE_SEARCH;
      case Step::ActivatingCube:
      case Step::PickingUpCube:
      case Step::PuttingCubeDown:
        return external_interface::STEP_CUBE_TRICK;
      case Step::Invalid:
      case Step::Complete:
        return external_interface::STEP_INVALID;
    }
  }
  
private:
  
  void TransitionToLookingAtUser()
  {
    DebugTransition("Waiting on continue to begin");
    _step = Step::LookingAtUser;
    _selectedBehavior = _behaviors[_step];
  }
  
  void TransitionToLookingForCube( BehaviorExternalInterface& bei )
  {
    DebugTransition("Looking for cube");
    _step = Step::LookingForCube;
    _selectedBehavior = _behaviors[_step];
    _cubeKnownCondition->SetActive( bei, true ); // no deactivate needed since this gets destroyed when the stage ends
    _timeStartedSearching_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    auto* gi = bei.GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ){
      auto* seesCubeMessage = new external_interface::OnboardingSeesCube{ false };
      gi->Broadcast( ExternalMessageRouter::Wrap(seesCubeMessage) );
    }
  }
  
  void TransitionToActivatingCube( BehaviorExternalInterface& bei )
  {
    auto* gi = bei.GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ){
      auto* seesCubeMessage = new external_interface::OnboardingSeesCube{ true };
      gi->Broadcast( ExternalMessageRouter::Wrap(seesCubeMessage) );
    }
    
    
    _activationBehavior->SetTargetID( _objectID );
    _step = Step::ActivatingCube;
    _selectedBehavior = _behaviors[_step];
    if( !_selectedBehavior->WantsToBeActivated() ) {
      // this can happen if it already ran to completion
      TransitionToPickingUpCube( bei );
    } else {
      DebugTransition("Activating cube");
      // only require confirmation on first attempt
      _activationBehavior->SetRequireConfirmation( _activationAttemptCount == 0 );
    }
  }
  
  void TransitionToPickingUpCube( BehaviorExternalInterface& bei )
  {
    DebugTransition("Picking up cube");
    
    auto& drivingAnimHandler = bei.GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.PushDrivingAnimations({
      AnimationTrigger::OnboardingCubeDriveGetIn,
      AnimationTrigger::OnboardingCubeDriveLoop,
      AnimationTrigger::OnboardingCubeDriveGetOut
    }, kLockName);
    
    _pickUpBehavior->SetTargetID( _objectID );
    _step = Step::PickingUpCube;
    _selectedBehavior = _behaviors[_step];
  }
  
  void TransitionToPuttingDownCube( BehaviorExternalInterface& bei )
  {
    if( (_putDownBehavior != nullptr) && _putDownBehavior->WantsToBeActivated() ) {
      DebugTransition("Putting down cube");
      _step = Step::PuttingCubeDown;
      _selectedBehavior = _behaviors[_step];
    } else {
      TransitionToComplete( bei );
    }
  }
  
  void TransitionToComplete( BehaviorExternalInterface& bei )
  {
    auto& drivingAnimHandler = bei.GetRobotInfo().GetDrivingAnimationHandler();
    drivingAnimHandler.RemoveDrivingAnimations( kLockName );
    
    DebugTransition("Complete");
    _step = Step::Complete;
    _selectedBehavior = nullptr;
  }
  
  bool HasCubeBeenSeenRecently( BehaviorExternalInterface& bei ) const
  {
    // check if the cube has been seen recently. todo: this may need to be ConnectedActive object
    // or something based on cube connection logic
    const auto* cubeObject = bei.GetBlockWorld().GetLocatedObjectByID(_objectID);
    if( cubeObject != nullptr ) {
      TimeStamp_t lastObservedTime_ms = cubeObject->GetLastObservedTime();
      TimeStamp_t currTime_ms = bei.GetRobotInfo().GetLastMsgTimestamp();
      if( currTime_ms <= kMaxAgeForRedoSearch_ms + lastObservedTime_ms ) {
        return true;
      }
    }
    return false;
  }
  
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.Cube", "%s", debugInfo.c_str());
  }
  
  enum class Step : uint8_t {
    Invalid=0,
    LookingAtUser,
    LookingForCube,
    ActivatingCube,
    PickingUpCube,
    PuttingCubeDown,
    Complete,
  };
  
  Step _step = Step::Invalid;
  IBehavior* _selectedBehavior = nullptr;
  
  ObjectID _objectID;
  
  unsigned int _activationAttemptCount;
  unsigned int _pickUpAttemptCount;
  TimeStamp_t _timeStartedSearching_ms;
  
  
  std::unordered_map<Step, IBehavior*> _behaviors;
  std::shared_ptr<BehaviorPickUpCube> _pickUpBehavior;
  std::shared_ptr<BehaviorOnboardingActivateCube> _activationBehavior;
  ICozmoBehaviorPtr _putDownBehavior;
  
  std::unique_ptr<ConditionObjectKnown> _cubeKnownCondition;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
