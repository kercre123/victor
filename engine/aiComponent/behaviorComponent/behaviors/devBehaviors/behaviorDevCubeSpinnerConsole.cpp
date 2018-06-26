/**
 * File: BehaviorDevCubeSpinnerConsole.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-21
 *
 * Description: A behavior for testing cube spinner via the web console
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevCubeSpinnerConsole.h"

#include "clad/types/animationTrigger.h"
#include "engine/activeObject.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/cubeSpinner/cubeSpinnerGame.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/dataAccessorComponent.h"

#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {
  
namespace {
bool s_LockLightNextTick = false;
void LockLight(ConsoleFunctionContextRef context)
{
  s_LockLightNextTick = true;
}

CONSOLE_FUNC(LockLight, "CubeSpinnerDev");

const char* kGameConfigKey = "gameConfig";

}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevCubeSpinnerConsole::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevCubeSpinnerConsole::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevCubeSpinnerConsole::BehaviorDevCubeSpinnerConsole(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.gameConfig = config[kGameConfigKey];

  SubscribeToTags({
    EngineToGameTag::ObjectTapped
  });

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevCubeSpinnerConsole::~BehaviorDevCubeSpinnerConsole()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevCubeSpinnerConsole::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kGameConfigKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::InitBehavior()
{
  const auto& lightConfig = GetBEI().GetDataAccessorComponent().GetCubeSpinnerConfig();

  _iConfig.cubeSpinnerGame = std::make_unique<CubeSpinnerGame>(_iConfig.gameConfig, lightConfig,  
                                                               GetBEI().GetCubeLightComponent(), 
                                                               GetBEI().GetBackpackLightComponent(), 
                                                               GetBEI().GetRobotInfo().GetRNG());
  
  auto lockedCallback = [this](CubeSpinnerGame::LockResult result){
    _dVars.lastLockResult = result;
  };

  _iConfig.cubeSpinnerGame->RegisterLightLockedCallback(lockedCallback);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  const ActiveObject* obj = GetBEI().GetBlockWorld().FindConnectedActiveMatchingObject(filter);
  if(obj != nullptr){
    _dVars.objID = obj->GetID();
    _iConfig.cubeSpinnerGame->StartNewGame(_dVars.objID);
  }else{
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  if(s_LockLightNextTick){
    _iConfig.cubeSpinnerGame->LockNow();
    s_LockLightNextTick = false;
  }

  _iConfig.cubeSpinnerGame->Update();

  if(_dVars.lastLockResult != CubeSpinnerGame::LockResult::Count){
    switch(_dVars.lastLockResult){
      case CubeSpinnerGame::LockResult::Locked:{
        DelegateNow(new TriggerAnimationAction(AnimationTrigger::PounceSuccess));
        break;
      }
      case CubeSpinnerGame::LockResult::Error:{
        DelegateNow(new TriggerAnimationAction(AnimationTrigger::PounceFail), [this](){
          _iConfig.cubeSpinnerGame->StopGame();
          _iConfig.cubeSpinnerGame->StartNewGame(_dVars.objID);
        });
        break;
      }
      case CubeSpinnerGame::LockResult::Complete:{
        DelegateNow(new TriggerAnimationAction(AnimationTrigger::FistBumpSuccess), [this](){
          _iConfig.cubeSpinnerGame->StopGame();
          _iConfig.cubeSpinnerGame->StartNewGame(_dVars.objID);
        });
        break;
      }
      default:{
        PRINT_NAMED_ERROR("BehaviorDevCubeSpinnerConsole.BehaviorUpdate.UnknownLockState", "");
        break;
      }
    }

    _dVars.lastLockResult = CubeSpinnerGame::LockResult::Count;
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevCubeSpinnerConsole::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::ObjectTapped:
    {
      if(event.GetData().Get_ObjectTapped().objectID == _dVars.objID ){
        _iConfig.cubeSpinnerGame->LockNow();
      }
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("BehaviorDevCubeSpinnerConsole.HandleWhileActivated.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}


}
}
