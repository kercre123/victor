/**
 * File: BehaviorBoxDemo.cpp
 *
 * Author: Brad
 * Created: 2019-01-03
 *
 * Description: Main state machine for the box demo
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemo.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::BehaviorBoxDemo(const Json::Value& config)
 : InternalStatesBehavior( config, CreateCustomConditions() )
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::~BehaviorBoxDemo()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CustomBEIConditionHandleList BehaviorBoxDemo::CreateCustomConditions()
{
  CustomBEIConditionHandleList handles;

  // TODO:(bn) put custom conditions here
  
  return handles;
}


}
}
