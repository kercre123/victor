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

#include "engine/aiComponent/behaviorComponent/userIntentMap.h"

#include "clad/types/featureGateTypes.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"
#include "util/featureGate/featureGate.h"
#include "util/logging/logging.h"

#include <unordered_set>

namespace Anki {
namespace Cozmo {

namespace {

const char* kUserIntentMapKey = "user_intent_map";
const char* kCloudIntentKey = "cloud_intent";
const char* kAppIntentKey = "app_intent";
const char* kUserIntentKey = "user_intent";
const char* kUnmatchedKey = "unmatched_intent";
const char* kCloudVariableSubstitutionsKey = "cloud_substitutions";
const char* kCloudVariableNumericsKey = "cloud_numerics";
const char* kAppVariableSubstitutionsKey = "app_substitutions";
const char* kAppVariableNumericsKey = "app_numerics";
const char* kFeatureGateKey = "feature_gate";

const char* kDebugName = "UserIntentMap";
}

UserIntentMap::UserIntentMap(const Json::Value& config, const CozmoContext* ctx)
  : _unmatchedUserIntent{ USER_INTENT(unmatched_intent) }
{
  // for validation
  std::unordered_set<UserIntentTag> foundUserIntent;
  std::unordered_set<UserIntentTag> gatedUserIntent;
  
  ANKI_VERIFY(config[kUserIntentMapKey].size() > 0,
              "UserIntentMap.InvalidConfig",
              "expected to find group '%s'",
              kUserIntentMapKey);

  for( const auto& mapping : config[kUserIntentMapKey] ) {
    const std::string& userIntentStr = JsonTools::ParseString(mapping, kUserIntentKey, kDebugName);
    
    UserIntentTag intentTag;
    ANKI_VERIFY( UserIntentTagFromString( userIntentStr, intentTag ),
                 "UserIntentMap.Ctor.InvalidIntent",
                 "supplied %s '%s' is invalid",
                 kUserIntentKey,
                 userIntentStr.c_str() );
    
    // check if this user intent is enabled (feature gate), and skip it if not
    {
      const std::string& featureName = mapping.get( kFeatureGateKey, "" ).asString();
      if( !featureName.empty() && (ctx != nullptr) ) {
        FeatureType feature = FeatureType::Invalid;
        ANKI_VERIFY( FeatureTypeFromString( featureName, feature ),
                     "UserIntentMap.Ctor.InvalidFeature",
                     "Unknown feature gate type '%s' for intent '%s'",
                     featureName.c_str(),
                     userIntentStr.c_str() );
        const bool featureEnabled = ctx->GetFeatureGate()->IsFeatureEnabled( feature );
        if( !featureEnabled ) {
          PRINT_NAMED_INFO( "UserIntentMap.Ctor.FeatureGated",
                            "Disabled feature '%s' is gating intent '%s'",
                            featureName.c_str(),
                            userIntentStr.c_str() );
          // this will be an unmatched intent!
          gatedUserIntent.insert( intentTag );
          continue;
        }
      }
    }
    
    foundUserIntent.insert( intentTag );
    
    auto processMapping = [&intentTag](const Json::Value& input,
                                       MapType& container,
                                       const char* intentKey,
                                       const char* subsKey,
                                       const char* numericsKey,
                                       const char* debugName)
    {
      const std::string& intentName = input.get(intentKey, "").asString();
      if( !intentName.empty() ) {
        VarSubstitutionList varSubstitutions;
        
        const auto& subs = input[subsKey];
        if( !subs.isNull() ) {
          for( const auto& fromStr : subs.getMemberNames() ) {
            const auto& to = subs[fromStr];
            if( ANKI_VERIFY( !to.isNull(), "UserIntentMap.Ctor.WTF", "Missing value") ) {
              const auto& toStr = to.asString();
              
              varSubstitutions.emplace_back( SanitationActions{fromStr, toStr, false} );
            }
          }
        }

        const auto& numerics = input[numericsKey];
        if( !numerics.isNull() ) {
          for( const auto& from : numerics ) {
            const auto& fromStr = from.asString();
            auto it = std::find_if( varSubstitutions.begin(), varSubstitutions.end(), [&fromStr](const auto& s) {
              return (s.from == fromStr);
            });
            if( it != varSubstitutions.end() ) {
              it->isNumeric = true;
            } else {
              varSubstitutions.emplace_back( SanitationActions{fromStr, "", true} );
            }
          }
        }
        
        // a cloud/app intent should only appear once, since it only has one user intent (although multiple
        // cloud/app intents can map to the same user intent
        ANKI_VERIFY( container.find(intentName) == container.end(),
                     "UserIntentMap.Ctor.MultipleAssociations",
                     "The %s' intent '%s' is already mapped to user intent '%s'",
                     debugName,
                     intentName.c_str(),
                     UserIntentTagToString( container.find(intentName)->second.userIntent ) );

        container.emplace(intentName, IntentInfo{intentTag, varSubstitutions});

        // TODO:(bn) remove as part of VIC-3438
        // Chipper is changing names of some intents to include _extend at the end. Until that change goes live,
        // add a hack to duplicate keys which end with _extend_ to also support being sent without _extent. Once
        // chipper is updated, we can remove this code, but this code is safe to go live before the chipper update
        // and will future proof robots during that update
        static const std::string kExtendEnding = "_extend";
        // credit: https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
        auto endsWith = [](std::string const & value, std::string const & ending) {
          if (ending.size() > value.size()) {
            return false;
          }
          return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        };
        
        if( endsWith(intentName, kExtendEnding) ) {
          std::string intentWithoutExtend = intentName.substr(0, intentName.length() - kExtendEnding.length());
          container.emplace( intentWithoutExtend, IntentInfo{intentTag, varSubstitutions});
        }
      }
    };
    
    processMapping( mapping, _cloudToUserMap, kCloudIntentKey, kCloudVariableSubstitutionsKey, kCloudVariableNumericsKey, "cloud" );
    processMapping( mapping, _appToUserMap, kAppIntentKey, kAppVariableSubstitutionsKey, kAppVariableNumericsKey, "app" );
    
    if( ANKI_DEVELOPER_CODE ) {
      // prevent typos
      std::vector<const char*> validKeys = {
        kCloudIntentKey, kCloudVariableSubstitutionsKey, kCloudVariableNumericsKey,
        kAppIntentKey, kAppVariableSubstitutionsKey, kAppVariableNumericsKey,
        kUserIntentKey, kFeatureGateKey,
      };
      std::vector<std::string> badKeys;
      const bool hasBadKeys = JsonTools::HasUnexpectedKeys( mapping, validKeys, badKeys );
      if( hasBadKeys ) {
        std::string keys;
        for( const auto& key : badKeys ) {
          keys += key;
          keys += ",";
        }
        DEV_ASSERT_MSG( false,
                        "UserIntentMap.Ctor.UnexpectedKey",
                        "Keys '%s' unexpected for intent '%s'",
                        keys.c_str(),
                        UserIntentTagToString(intentTag) );
      }
    }
  }

  std::string unmatchedString = JsonTools::ParseString(config, kUnmatchedKey, kDebugName);
  ANKI_VERIFY( UserIntentTagFromString(unmatchedString, _unmatchedUserIntent),
               "UserIntentMap.Ctor.InvalidUnmatchedIntent",
               "supplied %s '%s' is invalid",
               kUnmatchedKey,
               unmatchedString.c_str() );
  
  // now verify that all user intents were listed. This isnt necessary for the c++ code base, but we
  // want to keep the json accurate so that if anyone else cares about our user intents, they can
  // parse json instead of clad.
  if( ANKI_DEVELOPER_CODE ) {
    // ignore this only for those tests that specify a new mapping. Most tests will run with the
    // real deal data.
    if( config["is_test"].isNull() || !config["is_test"].asBool() ) {
      using Tag = std::underlying_type<UserIntentTag>::type;
      for( Tag i{0}; i<static_cast<Tag>( USER_INTENT(test_SEPARATOR) ); ++i ) {
        if( i != static_cast<Tag>(_unmatchedUserIntent) ) {
          DEV_ASSERT_MSG( foundUserIntent.find(static_cast<UserIntentTag>(i)) != foundUserIntent.end()
                       || gatedUserIntent.find(static_cast<UserIntentTag>(i)) != gatedUserIntent.end(),
                          "UserIntentMap.Ctor.MissingUserIntents",
                          "Every user intent found in clad must appear in json. You're missing '%s'",
                          UserIntentTagToString( static_cast<UserIntentTag>(i)) );
        }
      }
    }
  }
}

UserIntentTag UserIntentMap::GetUserIntentFromCloudIntent(const std::string& cloudIntent) const
{
  using Tag = std::underlying_type<UserIntentTag>::type;
  auto it = _cloudToUserMap.find(cloudIntent);
  if( it != _cloudToUserMap.end() ) {
    DEV_ASSERT( static_cast<Tag>( it->second.userIntent ) < static_cast<Tag>( USER_INTENT(INVALID) ), "Invalid intent value" );
    return it->second.userIntent;
  }
  else {
    DEV_ASSERT( static_cast<Tag>(_unmatchedUserIntent) < static_cast<Tag>(USER_INTENT(INVALID)), "Invalid intent value" );
    PRINT_NAMED_WARNING("UserIntentMap.NoCloudIntentMatch",
                        "No match for cloud intent '%s', returning default user intent '%s'",
                        cloudIntent.c_str(),
                        UserIntentTagToString(_unmatchedUserIntent));
    return _unmatchedUserIntent;
  }
}

bool UserIntentMap::IsValidCloudIntent(const std::string& cloudIntent) const
{
  const bool found = ( _cloudToUserMap.find(cloudIntent) != _cloudToUserMap.end() );
  return found;
}
  
UserIntentTag UserIntentMap::GetUserIntentFromAppIntent(const std::string& appIntent) const
{
  using Tag = std::underlying_type<UserIntentTag>::type;
  auto it = _appToUserMap.find(appIntent);
  if( it != _appToUserMap.end() ) {
    DEV_ASSERT( static_cast<Tag>( it->second.userIntent ) < static_cast<Tag>( USER_INTENT(INVALID) ), "Invalid intent value" );
    return it->second.userIntent;
  }
  else {
    DEV_ASSERT( static_cast<Tag>(_unmatchedUserIntent) < static_cast<Tag>(USER_INTENT(INVALID)), "Invalid intent value" );
    PRINT_NAMED_WARNING("UserIntentMap.NoAppIntentMatch",
                        "No match for app intent '%s', returning default user intent '%s'",
                        appIntent.c_str(),
                        UserIntentTagToString(_unmatchedUserIntent));
    return _unmatchedUserIntent;
  }
}
  
bool UserIntentMap::IsValidAppIntent(const std::string& appIntent) const
{
  const bool found = ( _appToUserMap.find(appIntent) != _appToUserMap.end() );
  return found;
}

void UserIntentMap::SanitizeCloudIntentVariables(const std::string& cloudIntent, Json::Value& paramsList) const
{
  SanitizeVariables(cloudIntent, _cloudToUserMap, "cloud", paramsList);
}
  
void UserIntentMap::SanitizeAppIntentVariables(const std::string& appIntent, Json::Value& paramsList) const
{
  SanitizeVariables(appIntent, _appToUserMap, "app", paramsList);
}
  
void UserIntentMap::SanitizeVariables(const std::string& intent,
                                      const MapType& container,
                                      const char* debugName,
                                      Json::Value& paramsList) const
{
  const auto it = container.find( intent );
  if( it != container.end() ) {
    const auto& subs = it->second.varSubstitutions;
    for( const auto& varName : paramsList.getMemberNames() ) {
      const auto* varNameP = &varName;
      const auto it = std::find_if( subs.begin(), subs.end(), [&varName](const auto& p) {
        return p.from == varName;
      });
      if( it != subs.end() ) {
        if( !it->to.empty() ) {
          // ignore it if the TO variable name already exists
          if( !ANKI_VERIFY( paramsList[it->to].isNull(),
                            "UserIntentMap.SanitizeVariables.InvalidSubs",
                            "%s intent '%s' variable substitutions are invalid",
                            debugName,
                            intent.c_str() ) )
          {
            continue;
          }
          
          if( it->to == it->from ) {
            PRINT_NAMED_WARNING( "UserIntentMap.SanitizeVariables.NoOp",
                                 "The provided '%s' substitution '%s' resulted in no op. Substitution is optional",
                                 debugName,
                                 it->to.c_str() );
          }
          
          // replace
          paramsList[it->to] = paramsList[varName];
          paramsList.removeMember( it->from );
          varNameP = &it->to; // since varname changed, update the pointer for the next block of code
        }

        if( it->isNumeric ) {
          // varNameP is the current var name, since it may have switched
          auto str = JsonTools::ParseString( paramsList, varNameP->c_str(), debugName );
          try {
            // replace a string type with its numeric type (string or long long)
            if( str.find(".") != std::string::npos ) {
              paramsList[*varNameP] = std::stof(str);
            } else {
              paramsList[*varNameP] = std::stoll(str);
            }
          } catch( std::exception ) {
            PRINT_NAMED_ERROR( "UserIntentMap.SanitizeVariables.Invalid",
                               "Tried to convert %s intent '%s' and failed. No conversion performed",
                               debugName,
                               str.c_str() );
          }
        }
      }
    }
  }
}
  
std::vector<std::string> UserIntentMap::DevGetCloudIntentsList() const
{
  std::vector<std::string> ret;
  ret.reserve( _cloudToUserMap.size() );
  for( const auto& pair : _cloudToUserMap ) {
    ret.push_back( pair.first );
  }
  return ret;
}

std::vector<std::string> UserIntentMap::DevGetAppIntentsList() const
{
  std::vector<std::string> ret;
  ret.reserve( _appToUserMap.size() );
  for( const auto& pair : _appToUserMap ) {
    ret.push_back( pair.first );
  }
  return ret;
}

  
}
}
