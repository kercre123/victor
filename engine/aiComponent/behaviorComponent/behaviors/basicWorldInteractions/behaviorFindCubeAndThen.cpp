/**
 * File: BehaviorFindCubeAndThen.cpp
 *
 * Author: Sam Russell
 * Created: 2018-08-20
 *
 * Description: Run BehaviorFindCube. Upon locating a cube, go to the pre-dock pose if found/known, connect to the cube,
 *              (visiblly if this behavior was user driven) then delegate to the followUp behavior, if specified.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCubeAndThen.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCube.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
                          _dVars.state = FindCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
const char* const kFollowUpBehaviorKey = "followUpBehaviorID";
const char* const kSkipConnectToCubeBehaviorKey = "skipConnectToCubeBehavior";
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::InstanceConfig::InstanceConfig()
: skipConnectToCubeBehavior(true) //default to true and override for user driven instances
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::DynamicVariables::DynamicVariables()
: state(FindCubeState::FindCube)
, cubeID()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::BehaviorFindCubeAndThen(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kFollowUpBehaviorKey, _iConfig.followUpBehaviorID);
  JsonTools::GetValueOptional(config, kSkipConnectToCubeBehaviorKey, _iConfig.skipConnectToCubeBehavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::~BehaviorFindCubeAndThen()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindCubeAndThen::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = false; 
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.findCubeBehavior.get());
  delegates.insert(_iConfig.connectToCubeBehavior.get());
  if(nullptr != _iConfig.followUpBehavior){
    delegates.insert(_iConfig.followUpBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kFollowUpBehaviorKey,
    kSkipConnectToCubeBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast<BehaviorFindCube>(BEHAVIOR_ID(FindCube),
                                                   BEHAVIOR_CLASS(FindCube),
                                                   _iConfig.findCubeBehavior);
  DEV_ASSERT(nullptr != _iConfig.findCubeBehavior,
             "BehaviorFindCubeAndThen.InitBehavior.NullFindCubeBehavior");
             
  _iConfig.connectToCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ConnectToCube));
  DEV_ASSERT(nullptr != _iConfig.connectToCubeBehavior,
             "BehaviorFindCubeAndThen.InitBehavior.NullConnectToCubeBehavior");

  if(!_iConfig.followUpBehaviorID.empty()){
    _iConfig.followUpBehavior = FindBehavior(_iConfig.followUpBehaviorID);
    DEV_ASSERT(nullptr != _iConfig.followUpBehavior,
              "BehaviorFindCubeAndThen.InitBehavior.NullFollowUpBehavior");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  TransitionToFindCube();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToFindCube()
{
  SET_STATE(FindCube);

  if(_iConfig.findCubeBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.findCubeBehavior.get(),
      [this](){
        _dVars.cubeID = _iConfig.findCubeBehavior->GetFoundCubeID();
        auto* targetCube = GetTargetCube(); 
        if(nullptr != targetCube){
          TransitionToDriveToPredockPose();
        } //else just exit, the get-out will have been handled by the findCubeBehavior since it didn't find anything
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToDriveToPredockPose()
{
  SET_STATE(DriveToPredockPose);

  DelegateIfInControl( new DriveToObjectAction( _dVars.cubeID, PreActionPose::ActionType::DOCKING ),
                       &BehaviorFindCubeAndThen::TransitionToAttemptConnection );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToAttemptConnection()
{
  SET_STATE(AttemptConnection);

  if(_iConfig.skipConnectToCubeBehavior){
    auto& ccc = GetBEI().GetCubeConnectionCoordinator();
    ccc.SubscribeToCubeConnection(this);
    TransitionToFollowUpBehavior();
  }
  else{
    if(_iConfig.connectToCubeBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.connectToCubeBehavior.get(),
        [this](){
          auto& ccc = GetBEI().GetCubeConnectionCoordinator();
          if(ccc.IsConnectedToCube()){
            // Convert the background subscription to foreground
            ccc.SubscribeToCubeConnection(this);
          }

          TransitionToFollowUpBehavior();
        });
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToFollowUpBehavior()
{
  SET_STATE(FollowUpBehavior);
  if(nullptr != _iConfig.followUpBehavior && _iConfig.followUpBehavior->WantsToBeActivated()){
    // Let the behavior end when the followup does
    DelegateIfInControl(_iConfig.followUpBehavior.get());
  }
  // If there is no followUp behavior, end now
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToGetOutFailure()
{
  SET_STATE(GetOutFailure);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFailure));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObservableObject* BehaviorFindCubeAndThen::GetTargetCube()
{
  return GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.cubeID);
}

} // namespace Vector
} // namespace Anki
