/**
 * File: conditionConnectedToCube.cpp
 *
 * Author: Sam Russell
 * Created: 2018 August 3
 *
 * Description: Condition that checks for a cube connection of the specified type (interactable/background)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionConnectedToCube.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"

namespace Anki{
namespace Vector{

namespace{
  static const char* kRequiredConnectionTypeKey = "connectionType";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionConnectedToCube::ConditionConnectedToCube(const Json::Value& config)
: IBEICondition(config)
, _requiredConnectionType(CubeConnectionType::Unspecified)
{
  std::string connectionTypeString;
  if(JsonTools::GetValueOptional(config, kRequiredConnectionTypeKey, connectionTypeString)){
    if(!EnumFromString(connectionTypeString, _requiredConnectionType)){
      PRINT_NAMED_ERROR("ConditionConnectedToCube.InvalidConnectionType",
                        "%s is an invalid connection type, defaulting to %s",
                        connectionTypeString.c_str(),
                        EnumToString(CubeConnectionType::Unspecified));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionConnectedToCube::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  auto& ccc = bei.GetCubeConnectionCoordinator();
  switch(_requiredConnectionType){
    case CubeConnectionType::Unspecified:
    {
      return ccc.IsConnectedToCube();
    }
    case CubeConnectionType::Interactable:
    {
      return ccc.IsConnectedInteractable();
    }
    case CubeConnectionType::Background:
    {
      return ccc.IsConnectedBackground();
    }
  }
}

} // namespace Vector
} // namespace Anki
