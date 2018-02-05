/**
 * File: userIntentMap.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-30
 *
 * Description: Mapping from other intents (e.g. cloud) to user intents
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/userIntentMap.h"

#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kUserIntentMapKey = "user_intent_map";
const char* kCloudIntentKey = "cloud_intent";
const char* kUserIntentKey = "user_intent";
const char* kUnmatchedKey = "unmatched_intent";
const char* kExtraDataKey = "extra_data";

const char* kDebugName = "UserIntentMap";
}

UserIntentMap::UserIntentMap(const Json::Value& config)
{
  ANKI_VERIFY(config[kUserIntentMapKey].size() > 0,
              "UserIntentMap.InvalidConfig",
              "expected to find group '%s'",
              kUserIntentMapKey);

  for( const auto& mapping : config[kUserIntentMapKey] ) {
    const std::string& cloudIntent = JsonTools::ParseString(mapping, kCloudIntentKey, kDebugName);
    const std::string& userIntent = JsonTools::ParseString(mapping, kUserIntentKey, kDebugName);
    const std::string& extraData = mapping.get(kExtraDataKey, "").asString();

    // TODO:(bn) we need a way to verify that extraData is correct and maps to a union tag. Unfortunately,
    // CLAD doesn't provide a way to do that right now. Ideally we'd enable EnumFromString on the enum data
    // tags, then we can check the return type there to verify        
    
    _cloudToUserMap.emplace(cloudIntent, IntentInfo{userIntent, extraData});
    _userIntents.emplace(userIntent);
  }

  _unmatchedUserIntent = JsonTools::ParseString(config, kUnmatchedKey, kDebugName);
}

const std::string& UserIntentMap::GetUserIntentFromCloudIntent(const std::string& cloudIntent) const
{
  auto it = _cloudToUserMap.find(cloudIntent);
  if( it != _cloudToUserMap.end() ) {
    return it->second.userIntent;
  }
  else {
    PRINT_NAMED_WARNING("UserIntentMap.NoCloudIntentMatch",
                        "No match for cloud intent '%s', returning default user intent '%s'",
                        cloudIntent.c_str(),
                        _unmatchedUserIntent.c_str());
    return _unmatchedUserIntent;
  }
}

bool UserIntentMap::IsValidCloudIntent(const std::string& cloudIntent) const
{
  const bool found = ( _cloudToUserMap.find(cloudIntent) != _cloudToUserMap.end() );
  return found;
}

bool UserIntentMap::IsValidUserIntent(const std::string& userIntent) const
{
  const bool foundInMap = ( _userIntents.find(userIntent) != _userIntents.end() );
  const bool isDefault = userIntent == _unmatchedUserIntent;
  return foundInMap || isDefault;
}

const std::string& UserIntentMap::GetCloudIntentExtraData(const std::string& cloudIntent) const
{
  const auto it = _cloudToUserMap.find(cloudIntent);
  if( it != _cloudToUserMap.end() ) {
    return it->second.extraData;
  }
  else {
    static const std::string kEmptyVal;
    return kEmptyVal;
  }  
}

}
}
