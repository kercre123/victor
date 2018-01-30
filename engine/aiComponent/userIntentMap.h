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

#ifndef __Engine_AiComponent_UserIntentMap_H__
#define __Engine_AiComponent_UserIntentMap_H__

#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

#include <map>
#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class UserIntentMap : private Util::noncopyable
{
public:

  explicit UserIntentMap(const Json::Value& config);

  // returns a user intent that matches cloud intent. If none is found, it will return an intent signaling
  // that there was no match
  const std::string& GetUserIntentFromCloudIntent(const std::string& cloudIntent) const;

  // returns true if the specified intent exists in the map
  bool IsValidCloudIntent(const std::string& cloudIntent) const;
  bool IsValidUserIntent(const std::string& userIntent) const;

private:

  std::map<std::string, std::string> _cloudToUserMap;

  // TODO:(bn) better data structure to avoid double-storage
  std::set<std::string> _userIntents;

  std::string _unmatchedUserIntent;
};

}
}

#endif
