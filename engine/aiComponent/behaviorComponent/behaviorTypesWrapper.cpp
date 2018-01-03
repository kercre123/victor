/**
 * File: behaviorTypesWrapper.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-18
 *
 * Description: This file provides a set of wrappers around the CLAD types defined in BehaviorTypes.clad. In
 *              almost all cases, these wrappers should be used instead of the raw clad files to avoid very
 *              slow incremental builds. If you directly use the CLAD enums, even in a .cpp file, that file
 *              will need to be re-built every time a behavior ID is added. By using these wrappers, the
 *              incremental build time after touching BehaviorTypes.clad will be much more manageable
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

// only include this here in the cpp
#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Cozmo {
namespace BehaviorTypesWrapper {

BehaviorID BehaviorIDFromString(const std::string& name)
{
  return Anki::Cozmo::BehaviorIDFromString(name);
}

BehaviorClass BehaviorClassFromString(const std::string& name)
{
  return Anki::Cozmo::BehaviorClassFromString(name);
}

ExecutableBehaviorType ExecutableBehaviorTypeFromString(const std::string& name)
{
  return Anki::Cozmo::ExecutableBehaviorTypeFromString(name);
}

const char* BehaviorIDToString(const BehaviorID in)
{
  return Anki::Cozmo::BehaviorIDToString(in);
}
    
const char* BehaviorClassToString(const BehaviorClass in)
{
  return Anki::Cozmo::BehaviorClassToString(in);
}

const char* ExecutableBehaviorTypeToString(const ExecutableBehaviorType in)
{
  return Anki::Cozmo::ExecutableBehaviorTypeToString(in);
}

ExecutableBehaviorType GetDefaultExecutableBehaviorType()
{
  return ExecutableBehaviorType::Count;
}

}
}
}
