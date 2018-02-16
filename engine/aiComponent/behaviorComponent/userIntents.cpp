/**
 * File: userIntents.h
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


#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "clad/types/behaviorComponent/userIntent.h"

namespace Anki {
namespace Cozmo {
  
UserIntentTag UserIntentTagFromString(const std::string& name)
{
  UserIntentTag intent = UserIntentTag::INVALID;
  UserIntentTagFromString(name, intent);
  return intent;
}
  
bool IsValidUserIntentTag(const std::string& name)
{
  UserIntentTag intent;
  return UserIntentTagFromString(name, intent);
}

bool UserIntentTagFromString(const std::string& name, UserIntentTag& intent)
{
  // todo: add a clad EnumFromString for union tags
  Json::Value json;
  json["type"] = name;
  UserIntent data;
  if( data.SetFromJSON( json ) ) {
    intent = data.GetTag();
    return true;
  } else {
    return false;
  }
}
  
UserIntentTag GetUserIntentTag(const UserIntent& intent)
{
  return intent.GetTag();
}

}
}
