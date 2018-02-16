/**
 * File: userIntentMap.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-30
 *
 * Description: Mapping from other intents (e.g. cloud) to user intents
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_UserIntentMap_H__
#define __Engine_AiComponent_BehaviorComponent_UserIntentMap_H__

#include "engine/aiComponent/behaviorComponent/userIntents.h"

#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Anki {
namespace Cozmo {

  
class UserIntentMap : private Util::noncopyable
{
public:

  explicit UserIntentMap(const Json::Value& config);

  // returns a user intent that matches cloud intent. If none is found, it will return an intent signaling
  // that there was no match
  UserIntentTag GetUserIntentFromCloudIntent(const std::string& cloudIntent) const;

  // returns true if the specified intent exists in the map
  bool IsValidCloudIntent(const std::string& cloudIntent) const;
  
  // modify paramsList, replacing the cloud variable names with the user intent variable names, and
  // turning quoted numeric types like "123" into actual json numeric types (no quotes), based on
  // the config passed in the constructor
  void SanitizeCloudIntentVariables(const std::string& cloudIntent, Json::Value& paramsList) const;

private:

  struct SanitationActions {
    std::string from;
    std::string to; // if non-empty, the "from" variable name is replaced with "to"
    bool isNumeric=false; // if true, we replace variable values "90" with 90
  };

  using VarSubstitutionList = std::vector< SanitationActions >;
  
  struct IntentInfo {
    UserIntentTag userIntent;
    VarSubstitutionList varSubstitutions;
  };
  
  std::map<std::string, IntentInfo> _cloudToUserMap;

  UserIntentTag _unmatchedUserIntent;
};

}
}

#endif
