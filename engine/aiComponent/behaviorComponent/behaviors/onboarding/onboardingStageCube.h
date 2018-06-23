/**
 * File: onboardingStageCube.h
 *
 * Author: ross
 * Created: 2018-06-07
 *
 * Description: Onboarding logic unit for cube, which finds a cube, sees it, and does a trick
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/iOnboardingStage.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "proto/external_interface/messages.pb.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const unsigned int kMaxAttempts = 3;
  const float kMaxSearchDuration_s = 60.0f;
  const int kMaxAgeForRedoSearch_ms = 5000;
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
    // todo: there's no "seeing the cube" animation on the robot or cube lights, since I want to see what that looks
    // like before adding stage for it
    delegates.insert( BEHAVIOR_ID(OnboardingCubeTrick) );
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
    _behaviors[Step::DoingATrick]      = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingCubeTrick) );
    _behaviors[Step::PuttingCubeDown]  = GetBehaviorByID( bei, BEHAVIOR_ID(PutDownBlock) );
    
    bei.GetBehaviorContainer().FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingCubeTrick),
                                                            BEHAVIOR_CLASS(PickUpCube),
                                                            _pickUpBehavior );
    _putDownBehavior = bei.GetBehaviorContainer().FindBehaviorByID( BEHAVIOR_ID(PutDownBlock) );
    
    // initial behavior
    TransitionToLookingAtUser();
    
    // disable trigger word
    SetTriggerWordEnabled(false);
    
    _cubeKnownCondition->Init( bei );
    
    _attemptCount = 0;
    _timeStartedSearching_ms = 0;
  }
  
  virtual void OnContinue( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::LookingAtUser ) {
      TransitionToLookingForCube( bei );
    } else {
      DEV_ASSERT(false, "OnboardingStageCube.UnexpectedContinue");
    }
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    DebugTransition( "Skipped. Complete." );
    // all skips move to the next stage.
    _selectedBehavior = nullptr;
    _step = Step::Complete;
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::DoingATrick ) {
      const auto& whiteboard = bei.GetAIComponent().GetComponent<AIWhiteboard>();
      const float kTimeSinceFailure = 1.0f;
      const bool recentFail = whiteboard.DidFailToUse(_objectID,
                                                      AIWhiteboard::ObjectActionFailure::PickUpObject,
                                                      kTimeSinceFailure);
      if( recentFail && (_attemptCount < kMaxAttempts) ) {
        // check if the cube has been seen recently. todo: this may need to be ConnectedActive object
        // or something based on cube connection logic
        const auto* cubeObject = bei.GetBlockWorld().GetLocatedObjectByID(_objectID);
        if( cubeObject != nullptr ) {
          TimeStamp_t lastObservedTime_ms = cubeObject->GetLastObservedTime();
          TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
          if( currTime_ms <= kMaxAgeForRedoSearch_ms + lastObservedTime_ms ) {
            // use this cube
            TransitionToCubeTrick( bei );
          } else {
            TransitionToLookingForCube( bei );
          }
        } else {
          TransitionToLookingForCube( bei );
        }
      } else {
        // success! or silent (to the app) failure!
        if( recentFail ) {
          DebugTransition( "Trick Failed too many times. Complete." );
        } else {
          DebugTransition( "Trick Success." );
        }
        if( (_putDownBehavior != nullptr) && _putDownBehavior->WantsToBeActivated() ) {
          DebugTransition("Putting down cube");
          _step = Step::PuttingCubeDown;
          _selectedBehavior = _behaviors[_step];
        } else {
          DebugTransition("Complete");
          _step = Step::Complete;
          _selectedBehavior = nullptr;
        }
      }
    } else if( _step == Step::PuttingCubeDown ) {
      DebugTransition("Complete");
      _step = Step::Complete;
      _selectedBehavior = nullptr;
    }
    
    
  }
  // don't bother with handling interruptions. The only one that would return true (stage complete)
  // is if the robot is placed on the charger or hits low battery after the trick but before the
  // success animation finishes. Ignoeing that for now
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    _selectedBehavior = _behaviors[_step];
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
          TransitionToCubeTrick( bei );
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
      auto* seesCubeMessage = new external_interface::OnboardingSeesCube;
      seesCubeMessage->set_sees_cube( false );
      gi->Broadcast( ExternalMessageRouter::Wrap(seesCubeMessage) );
    }
  }
  
  void TransitionToCubeTrick( BehaviorExternalInterface& bei )
  {
    DebugTransition("Doing a trick");
    auto* gi = bei.GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ){
      auto* seesCubeMessage = new external_interface::OnboardingSeesCube;
      seesCubeMessage->set_sees_cube( true );
      gi->Broadcast( ExternalMessageRouter::Wrap(seesCubeMessage) );
    }
    
    // todo: cube lights animation here, or more likely in a new previous step
    
    _pickUpBehavior->SetTargetID( _objectID );
    _step = Step::DoingATrick;
    _selectedBehavior = _behaviors[_step];
    ++_attemptCount;
  }
  
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.Cube", "%s", debugInfo.c_str());
  }
  
  enum class Step : uint8_t {
    Invalid=0,
    LookingAtUser,
    LookingForCube,
    DoingATrick,
    PuttingCubeDown,
    Complete,
  };
  
  Step _step = Step::Invalid;
  IBehavior* _selectedBehavior = nullptr;
  
  ObjectID _objectID;
  
  unsigned int _attemptCount;
  TimeStamp_t _timeStartedSearching_ms;
  
  
  std::unordered_map<Step, IBehavior*> _behaviors;
  std::shared_ptr<BehaviorPickUpCube> _pickUpBehavior;
  ICozmoBehaviorPtr _putDownBehavior;
  
  std::unique_ptr<ConditionObjectKnown> _cubeKnownCondition;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageCube__
