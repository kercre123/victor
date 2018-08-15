/**
 * File: behaviorTypesWrapper.h
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

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorID_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorID_H__

#include "util/global/globalDefinitions.h"

#include <string>
#include <cstdint>

// if set to 1, use strings to avoid triggering massive rebuilds when clad changes
// otherwise, use the enums to guarantee compile-time errors for invalid ids
#define BEHAVIOR_ID_DEV_MODE ANKI_DEVELOPER_CODE

#if BEHAVIOR_ID_DEV_MODE
// forward declare behavior id rather than include
namespace Anki {
namespace Vector {
enum class BehaviorID : uint16_t;
enum class BehaviorClass : uint8_t;
enum class ExecutableBehaviorType : uint8_t;
const char* EnumToString( const BehaviorID id );
}
}
#else
#include "clad/types/behaviorComponent/behaviorTypes.h"
#endif

#if BEHAVIOR_ID_DEV_MODE
#define BEHAVIOR_ID(name) Anki::Vector::BehaviorTypesWrapper::BehaviorIDFromString(#name)
#define BEHAVIOR_CLASS(name) Anki::Vector::BehaviorTypesWrapper::BehaviorClassFromString(#name)
#else
#define BEHAVIOR_ID(name) Anki::Vector::BehaviorID::name
#define BEHAVIOR_CLASS(name) Anki::Vector::BehaviorClass::name
#endif

namespace Anki {
namespace Vector {
namespace BehaviorTypesWrapper {

BehaviorID BehaviorIDFromString(const std::string& name);
bool BehaviorIDFromString(const std::string& name, BehaviorID& id);
bool IsValidBehaviorID(const std::string& name);
BehaviorClass BehaviorClassFromString(const std::string& name);
ExecutableBehaviorType ExecutableBehaviorTypeFromString(const std::string& name);

const char* BehaviorIDToString(const BehaviorID in);
const char* BehaviorClassToString(const BehaviorClass in);
const char* BehaviorClassToString(const ExecutableBehaviorType in);
  
#if BEHAVIOR_ID_DEV_MODE
uint16_t GetBehaviorIDNumEntries();
#else
constexpr uint16_t GetBehaviorIDNumEntries() { return BehaviorIDNumEntries; }
#endif

ExecutableBehaviorType GetDefaultExecutableBehaviorType();

}
}
}

#endif
