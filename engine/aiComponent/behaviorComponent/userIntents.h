/**
 * File: userIntents.cpp
 *
 * Author: ross
 * Created: 2018 feb 12
 *
 * Description: This file provides a set of wrappers around the CLAD types and methods in userIntent.h.
 *              In almost all cases, these wrappers should be used instead of clad-generated code to
 *              avoid very slow incremental builds. If you directly use the CLAD enums, even in a
 *              .cpp file, that file will need to be re-built every time a UserIntent type is added
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_UserIntents_H__
#define __Engine_AiComponent_BehaviorComponent_UserIntents_H__

#include "util/global/globalDefinitions.h"

#include <cstdint>
#include <string>

#define USER_INTENT_DEV_MODE ANKI_DEVELOPER_CODE

#if !USER_INTENT_DEV_MODE
// include the real deal, but only the tag file, not the union
#include "clad/types/behaviorComponent/userIntentTag.h"
#endif

namespace Anki {
namespace Vector {

// always forward declare this one
class UserIntent;

#if USER_INTENT_DEV_MODE
  // forward declare user intent tags rather than include
  enum class UserIntentTag : uint8_t;
  enum class UserIntentSource : uint8_t;
  const char* UserIntentTagToString(const UserIntentTag tag);
#endif
}
}


#if USER_INTENT_DEV_MODE
#define USER_INTENT(name) Anki::Vector::UserIntentTagFromString(#name)
#else
#define USER_INTENT(name) Anki::Vector::UserIntentTag::name
#endif

namespace Anki {
namespace Vector {
  
UserIntentTag UserIntentTagFromString(const std::string& name);
bool UserIntentTagFromString(const std::string& name, UserIntentTag& intent);
bool IsValidUserIntentTag(const std::string& name);
  
bool IsValidUserIntentTag(UserIntentTag intent);
  
UserIntentTag GetUserIntentTag(const UserIntent& intent);
  
} // Cozmo
} // Anki


#endif // __Engine_AiComponent_BehaviorComponent_UserIntents_H__
