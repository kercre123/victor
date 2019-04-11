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

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/featureGateTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "json/json.h"
#include "util/featureGate/featureGate.h"
#include "util/logging/logging.h"

#include <unordered_set>

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {

const char* kUserIntentMapKey = "user_intent_map";
const char* kCloudIntentKey = "cloud_intent";
const char* kAppIntentKey = "app_intent";
const char* kUserIntentKey = "user_intent";
const char* kUnmatchedKey = "unmatched_intent";
const char* kTestUserIntentParsingKey = "test_parsing";
const char* kCloudVariableSubstitutionsKey = "cloud_substitutions";
const char* kCloudVariableNumericsKey = "cloud_numerics";
const char* kAppVariableSubstitutionsKey = "app_substitutions";
const char* kAppVariableNumericsKey = "app_numerics";
const char* kFeatureGateKey = "feature_gate";
const char* kSimpleVoiceResponsesKey = "simple_voice_responses";
const char* kResponseKey = "response";

const char* kDebugName = "UserIntentMap";
}

// defined in the cpp to hide UserIntent class definition from userIntentMap.h
struct SimpleVoiceResponseMap {
  std::map<std::string, UserIntent> map;
};

UserIntentMap::UserIntentMap(const Json::Value& config, const CozmoContext* ctx)
  : _simpleCloudResponseMap( new SimpleVoiceResponseMap )
  , _unmatchedUserIntent{ USER_INTENT(unmatched_intent) }
{
  // for validation
  std::unordered_set<UserIntentTag> foundUserIntent;
  std::unordered_set<UserIntentTag> gatedUserIntent;

  ANKI_VERIFY(config[kUserIntentMapKey].size() > 0,
              "UserIntentMap.InvalidConfig",
              "expected to find group '%s'",
              kUserIntentMapKey);

  // return true if this feature passes the gate (or no gate is enabled)
  auto featureGateOK = [ctx](const Json::Value& jsonGroup, const std::string& intentStr) {
    const std::string& featureName = jsonGroup.get( kFeatureGateKey, "" ).asString();
    if( !featureName.empty() && (ctx != nullptr) ) {
      FeatureType feature = FeatureType::Invalid;
      ANKI_VERIFY( FeatureTypeFromString( featureName, feature ),
                   "UserIntentMap.Ctor.InvalidFeature",
                   "Unknown feature gate type '%s' for intent '%s'",
                   featureName.c_str(),
                   intentStr.c_str() );
      const bool featureEnabled = ctx->GetFeatureGate()->IsFeatureEnabled( feature );
      if( !featureEnabled ) {
        LOG_INFO( "UserIntentMap.Ctor.FeatureGated",
                  "Disabled feature '%s' is gating intent '%s'",
                  featureName.c_str(),
                  intentStr.c_str() );
        return false;
        // this will be an unmatched intent!
      }
    }
    // feature gate passed or not specified, OK
    return true;
  };

  for( const auto& mapping : config[kUserIntentMapKey] ) {
    const std::string& userIntentStr = JsonTools::ParseString(mapping, kUserIntentKey, kDebugName);

    UserIntentTag intentTag;
    ANKI_VERIFY( UserIntentTagFromString( userIntentStr, intentTag ),
                 "UserIntentMap.Ctor.InvalidIntent",
                 "supplied %s '%s' is invalid",
                 kUserIntentKey,
                 userIntentStr.c_str() );

    if( !featureGateOK(mapping, userIntentStr) ) {
      // this will be an unmatched intent!
      gatedUserIntent.insert( intentTag );
      continue;
    }

    foundUserIntent.insert( intentTag );

    auto processMapping = [&intentTag](const Json::Value& input,
                                       MapType& container,
                                       const char* intentKey,
                                       const char* subsKey,
                                       const char* numericsKey,
                                       const char* testKey,
                                       const char* debugName)
    {
      const std::string& intentName = input.get(intentKey, "").asString();
      if( !intentName.empty() ) {
        const bool doTest = input.get(testKey, true).asBool();

        VarSubstitutionList varSubstitutions;

        const auto& subs = input[subsKey];
        if( !subs.isNull() ) {
          for( const auto& fromStr : subs.getMemberNames() ) {
            const auto& to = subs[fromStr];
            if( ANKI_VERIFY( !to.isNull(), "UserIntentMap.Ctor.InvalidConfig.MissingSubstitution", "Missing value") ) {
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

        container.emplace(intentName, IntentInfo{intentTag, varSubstitutions, doTest});
      }
    };

    processMapping( mapping, _cloudToUserMap, kCloudIntentKey, kCloudVariableSubstitutionsKey, kCloudVariableNumericsKey, kTestUserIntentParsingKey, "cloud" );
    processMapping( mapping, _appToUserMap, kAppIntentKey, kAppVariableSubstitutionsKey, kAppVariableNumericsKey, kTestUserIntentParsingKey, "app" );

    if( ANKI_DEVELOPER_CODE ) {
      // prevent typos
      std::vector<const char*> validKeys = {
        kCloudIntentKey, kCloudVariableSubstitutionsKey, kCloudVariableNumericsKey,
        kAppIntentKey, kAppVariableSubstitutionsKey, kAppVariableNumericsKey,
        kUserIntentKey, kFeatureGateKey, kTestUserIntentParsingKey, kSimpleVoiceResponsesKey, kResponseKey
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

  for( const auto& simpleResponseGroup : config[kSimpleVoiceResponsesKey] ) {

    const std::string& cloudIntent = JsonTools::ParseString(simpleResponseGroup, kCloudIntentKey, kDebugName);

    if( !featureGateOK(simpleResponseGroup, cloudIntent) ) {
      // skip this mapping because it is feature-flag disabled
      continue;
    }

    if( _cloudToUserMap.find(cloudIntent) != _cloudToUserMap.end() ) {
      LOG_ERROR("UserIntentMap.Ctor.InvalidConfig.DuplicateCloudIntent.Normal",
                "Cloud intent '%s' specified in %s is already defined as a normal response",
                cloudIntent.c_str(),
                kSimpleVoiceResponsesKey);
      continue;
    }

    if( _simpleCloudResponseMap->map.find(cloudIntent) != _simpleCloudResponseMap->map.end() ) {
      LOG_ERROR("UserIntentMap.Ctor.InvalidConfig.DuplicateCloudIntent.Simple",
                "Cloud intent '%s' specified in %s is already defined as another simple response",
                cloudIntent.c_str(),
                kSimpleVoiceResponsesKey);
      continue;
    }

    // set default feature to a "basic voice command"
    MetaUserIntent_SimpleVoiceResponse response;

    // manually set default
    response.anim_group = "";
    response.emotion_event = "";
    response.active_feature = ActiveFeature::BasicVoiceCommand;
    response.disable_wakeword_turn = false;

    const bool jsonOK = response.SetFromJSON( simpleResponseGroup[kResponseKey] );
    if( !jsonOK ) {
      LOG_ERROR("UserIntentMap.Ctor.InvalidConfig.SimpleResponseParseFail.Response",
                "Could not parse simple voice response for cloud intent '%s'",
                cloudIntent.c_str() );
      continue;
    }

    // NOTE: in order to check the validity of anim groups and emotion events, we need access to more
    // components than we have here. This is the job of the owner (i.e. UserIntentComponent)

    _simpleCloudResponseMap->map.emplace(cloudIntent, std::move(response));

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

UserIntentMap::~UserIntentMap()
{
}

void UserIntentMap::VerifySimpleVoiceResponses(UserIntentMap::SimpleVoiceResponseVerification verify,
                                               const std::string& debugKey)
{
  for( auto mapIt = _simpleCloudResponseMap->map.begin(); mapIt != _simpleCloudResponseMap->map.end(); ) {
    const auto& response = mapIt->second.Get_simple_voice_response();
    if( verify(response) ) {
      LOG_DEBUG("UserIntentMap.VerifySimpleVoiceResponses.Pass",
                "%s: response to cloud intent '%s' passed (verify returned true)",
                debugKey.c_str(),
                mapIt->first.c_str());

      ++mapIt;
    }
    else {
      LOG_ERROR("UserIntentMap.VerifySimpleVoiceResponses.Fail",
                "%s: response to cloud intent '%s' failed verification, removing from map",
                debugKey.c_str(),
                mapIt->first.c_str());

      mapIt = _simpleCloudResponseMap->map.erase( mapIt );
    }
  }
}



UserIntentTag UserIntentMap::GetUserIntentFromCloudIntent(const std::string& cloudIntent) const
{
  using Tag = std::underlying_type<UserIntentTag>::type;
  auto it = _cloudToUserMap.find(cloudIntent);
  if( it != _cloudToUserMap.end() ) {
    DEV_ASSERT( static_cast<Tag>( it->second.userIntent ) < static_cast<Tag>( USER_INTENT(INVALID) ),
                "Invalid intent value" );
    return it->second.userIntent;
  }
  else {

    // check if it's a simple cloud response
    auto simpleResponseIter = _simpleCloudResponseMap->map.find(cloudIntent);
    if( simpleResponseIter != _simpleCloudResponseMap->map.end() ) {
      return UserIntentTag::simple_voice_response;
    }
    else {

      DEV_ASSERT( static_cast<Tag>(_unmatchedUserIntent) < static_cast<Tag>(USER_INTENT(INVALID)),
                  "Invalid intent value" );
      LOG_WARNING("UserIntentMap.NoCloudIntentMatch",
                  "No match for cloud intent '%s', returning default user intent '%s'",
                  cloudIntent.c_str(),
                  UserIntentTagToString(_unmatchedUserIntent));
      return _unmatchedUserIntent;
    }
  }
}

bool UserIntentMap::IsValidCloudIntent(const std::string& cloudIntent) const
{
  const bool found = ( _cloudToUserMap.find(cloudIntent) != _cloudToUserMap.end() );
  return found;
}

bool UserIntentMap::GetTestParsingBoolFromCloudIntent(const std::string& cloudIntent) const
{
  auto it = _cloudToUserMap.find(cloudIntent);
  if( it != _cloudToUserMap.end() ) {
    return it->second.testParsing;
  }
  else {
    LOG_WARNING("UserIntentMap.NoCloudIntentMatch",
                "No match for cloud intent '%s', returning default value of true to test user intent parsing",
                cloudIntent.c_str());
    return true;
  }
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
    LOG_WARNING("UserIntentMap.NoAppIntentMatch",
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
            LOG_WARNING( "UserIntentMap.SanitizeVariables.NoOp",
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
            LOG_ERROR( "UserIntentMap.SanitizeVariables.Invalid",
                       "Tried to convert %s intent '%s' and failed. No conversion performed",
                       debugName,
                       str.c_str() );
          }
        }
      }
    }
  }
}

const UserIntent& UserIntentMap::GetSimpleVoiceResponse(const std::string& cloudIntent) const
{
  auto it = _simpleCloudResponseMap->map.find(cloudIntent);
  if( it == _simpleCloudResponseMap->map.end() ) {
    LOG_ERROR("UserIntentMap.GetSimpleVoiceResponse.NoSimpleResponse",
              "Cloud intent '%s' not found among the %zu intents in the simple response map",
              cloudIntent.c_str(),
              _simpleCloudResponseMap->map.size());
    static const UserIntent sDefaultIntent;
    return sDefaultIntent;
  }
  else {
    return it->second;
  }
}

std::vector<std::string> UserIntentMap::DevGetCloudIntentsList() const
{
  std::vector<std::string> ret;
  ret.reserve( _cloudToUserMap.size() + _simpleCloudResponseMap->map.size() );
  for( const auto& pair : _cloudToUserMap ) {
    ret.push_back( pair.first );
  }
  for( const auto& pair : _simpleCloudResponseMap->map ) {
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
