/**
 * File: blockConfigTypeHelpers.h
 *
 * Author: Kevin M. Karol
 * Created: 10/26/16
 *
 * Description: Helper functions for dealing with BlockConfigurations::ConfigurationType
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BlockWorld_BlockConfigTypeHelper_H__
#define __Cozmo_Basestation_BlockWorld_BlockConfigTypeHelper_H__

#include "util/enums/enumOperators.h"
#include "json/json-forwards.h"
#include <string>


namespace Anki {
namespace Cozmo {
namespace BlockConfigurations{

// forward declaration
enum class ConfigurationType;

DECLARE_ENUM_INCREMENT_OPERATORS(ConfigurationType);

ConfigurationType BlockConfigurationFromString(const char* inString, bool assertOnInvalidEnum = true);
  
bool IsBlockConfiguration(const char* inString);
  
const char* EnumToString(const ConfigurationType t);

} // namespace BlockConfigurations
} // namespace Cozmo
  
namespace JsonTools
{
  bool GetValueOptional(const Json::Value& config, const std::string& key, Cozmo::BlockConfigurations::ConfigurationType& value);

}
} // namespace Anki


#endif // __Cozmo_Basestation_Events_AnimationTriggerHelpers_H__

