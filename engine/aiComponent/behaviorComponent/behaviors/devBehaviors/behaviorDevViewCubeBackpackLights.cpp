/**
 * File: BehaviorDevViewCubeBackpackLights.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-24
 *
 * Description: A behavior which reads in a list of cube/backpack lights and exposes console vars to move through them
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevViewCubeBackpackLights.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace{
const char* kCubeTriggersKey = "cubeTriggers";
const char* kBackpackTriggersKey = "backpackTriggers";

#define CONSOLE_GROUP "DevViewLights"
CONSOLE_VAR(u32, kCubeTriggerIdx, CONSOLE_GROUP, 0);
CONSOLE_VAR(u32, kBackpackTriggerIdx, CONSOLE_GROUP, 0);
#undef CONSOLE_GROUP

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevViewCubeBackpackLights::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevViewCubeBackpackLights::DynamicVariables::DynamicVariables()
{
  // Offset by 1 to ensure first message gets sent
  lastCubeTriggerIdx =  kCubeTriggerIdx + 1;
  lastBackpackTriggerIdx = kCubeTriggerIdx + 1;
  lastCubeTrigger = CubeAnimationTrigger::Count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevViewCubeBackpackLights::BehaviorDevViewCubeBackpackLights(const Json::Value& config)
: ICozmoBehavior(config)
{
  if(ANKI_VERIFY(config[kCubeTriggersKey].isArray(), 
     "BehaviorDevViewCubeBackpackLights.Constructor.CubeTriggersNotArray","")){
    for(const auto& trigger: config[kCubeTriggersKey]){
      _iConfig.cubeTriggers.push_back(CubeAnimationTriggerFromString(trigger.asString()));
    }
  }

  if(ANKI_VERIFY(config[kBackpackTriggersKey].isArray(), 
     "BehaviorDevViewCubeBackpackLights.Constructor.BackpackTriggersNotArray","")){
    for(const auto& trigger: config[kBackpackTriggersKey]){
      _iConfig.backpackTriggers.push_back(BackpackAnimationTriggerFromString(trigger.asString()));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevViewCubeBackpackLights::~BehaviorDevViewCubeBackpackLights()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevViewCubeBackpackLights::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::GetAllDelegates(std::set<IBehavior*>& delegates) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCubeTriggersKey,
    kBackpackTriggersKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::OnBehaviorDeactivated()
{
  GetBEI().GetBackpackLightComponent().ClearAllBackpackLightConfigs();
  GetBEI().GetCubeLightComponent().StopAllAnims();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevViewCubeBackpackLights::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  // backpack lights
  if(kBackpackTriggerIdx != _dVars.lastBackpackTriggerIdx){
    const auto validIdx = std::min(kBackpackTriggerIdx, static_cast<u32>(_iConfig.backpackTriggers.size() - 1));
    GetBEI().GetBackpackLightComponent().SetBackpackAnimation(_iConfig.backpackTriggers[validIdx]);
    _dVars.lastBackpackTriggerIdx = kBackpackTriggerIdx;
  }

  // cube lights
  if(!_dVars.objectID.IsSet()){
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::LightCube);
    const ActiveObject* obj = GetBEI().GetBlockWorld().FindConnectedActiveMatchingObject(filter);

    if(obj != nullptr){
      _dVars.objectID = obj->GetID();
    }
  }

  if(!_dVars.objectID.IsSet()){
    return;
  }

  if(kCubeTriggerIdx != _dVars.lastCubeTriggerIdx){
    const auto validIdx = std::min(kCubeTriggerIdx, static_cast<u32>(_iConfig.cubeTriggers.size() - 1));
    const auto cubeTrigger = _iConfig.cubeTriggers[validIdx];
    auto animCompleteCallback = [this](){
      _dVars.lastCubeTrigger = CubeAnimationTrigger::Count;
    };
    
    if(_dVars.lastCubeTrigger == CubeAnimationTrigger::Count){

      GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(_dVars.objectID, cubeTrigger, animCompleteCallback);
    }else{
      GetBEI().GetCubeLightComponent().StopAndPlayLightAnim(_dVars.objectID, _dVars.lastCubeTrigger, cubeTrigger, animCompleteCallback);
    }
    _dVars.lastCubeTrigger = cubeTrigger;
    _dVars.lastCubeTriggerIdx = kCubeTriggerIdx;
  }



}

}
}
