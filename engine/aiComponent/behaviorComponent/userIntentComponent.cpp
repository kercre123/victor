/**
 * File: userIntentComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-02-01
 *
 * Description: Component to hold and query user intents (e.g. voice or app commands)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudServer.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/behaviorComponent/userIntentMap.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/userIntent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

// #include "webServerProcess/src/webService.h"


#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace {

static const size_t kMaxTicksToWarn = 2;
static const size_t kMaxTicksToClear = 3;
  
static const float kPreservedTimeToClear = 60*60*24;

static const char* kCloudIntentJsonKey = "intent";
static const char* kParamsKey = "params";
}
  
UserIntentComponent::UserIntentComponent(const Robot& robot, const Json::Value& userIntentMapConfig)
  : IDependencyManagedComponent( this, BCComponentID::UserIntentComponent )
  , _intentMap( new UserIntentMap(userIntentMapConfig) )
  , _context( robot.GetContext() )
{
  
  // setup cloud intent handler
  const auto& serverName = GetServerName( robot );
  _server.reset( new BehaviorComponentCloudServer( std::bind( &UserIntentComponent::OnCloudData,
                                                              this,
                                                              std::placeholders::_1),
                                                   serverName ) );
  
  // setup trigger word handler
  auto triggerWordCallback = [this]( const AnkiEvent<RobotInterface::RobotToEngine>& event ){
    SetTriggerWordPending();
  };
  if( robot.GetRobotMessageHandler() != nullptr ) {
    _eventHandles.push_back( robot.GetRobotMessageHandler()->Subscribe( RobotInterface::RobotToEngineTag::triggerWordDetected,
                                                                        triggerWordCallback ) );
  }
  
  // setup app intent handler
  if( robot.HasExternalInterface() ){
    using namespace ExternalInterface;
    auto onEvent = [this](const AnkiEvent<MessageGameToEngine>& event) {
      if( event.GetData().GetTag() == MessageGameToEngineTag::AppIntent ) {
        OnAppIntent( event.GetData().Get_AppIntent() );
      }
    };
    _eventHandles.push_back( robot.GetExternalInterface()->Subscribe( MessageGameToEngineTag::AppIntent, onEvent ));
  }
  
}

UserIntentComponent::~UserIntentComponent()
{
}


bool UserIntentComponent::IsTriggerWordPending() const
{
  return _pendingTrigger;
}

void UserIntentComponent::ClearPendingTriggerWord()
{
  if( !_pendingTrigger ) {
    PRINT_NAMED_WARNING("UserIntentComponent.ClearPendingTrigger.TriggerNotSet",
                        "Trying to clear trigger but the trigger isn't set. This is likely a bug");
  }
  else {
    _pendingTrigger = false;
  }
}

void UserIntentComponent::SetTriggerWordPending()
{
  if( _pendingTrigger ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetPendingTrigger.AlreadyPending",
                        "setting a pending trigger word but the last one hasn't been cleared");
  }

  _pendingTrigger = true;
  _pendingTriggerTick = BaseStationTimer::getInstance()->GetTickCount();
}

bool UserIntentComponent::IsAnyUserIntentPending() const
{
  return (_pendingIntent != nullptr);
}

bool UserIntentComponent::IsUserIntentPending(UserIntentTag userIntent) const
{
  return (_pendingIntent != nullptr) && (_pendingIntent->GetTag() == userIntent);
}

bool UserIntentComponent::IsUserIntentPending(UserIntentTag userIntent, UserIntent& extraData) const
{
  if( IsUserIntentPending(userIntent) ) {
    extraData = *_pendingIntent;
    return true;
  }
  else {
    return false;
  }
}

void UserIntentComponent::ClearUserIntent(UserIntentTag userIntent)
{
  if( (_pendingIntent != nullptr) && (userIntent != _pendingIntent->GetTag()) ) {
    PRINT_NAMED_WARNING("UserIntentComponent.ClearUserIntent.IncorrectIntent",
                        "Trying to clear intent '%s' but '%s' is pending. Not clearing",
                        UserIntentTagToString(userIntent),
                        UserIntentTagToString(_pendingIntent->GetTag()));
  }
  else if( _pendingIntent != nullptr ) {
    _pendingIntent.reset();
  }
}
  
UserIntent* UserIntentComponent::ClearUserIntentWithOwnership(UserIntentTag userIntent)
{
  if( (_pendingIntent != nullptr) && (userIntent != _pendingIntent->GetTag()) ) {
    PRINT_NAMED_WARNING( "UserIntentComponent.ClearUserIntentWithOwnership.IncorrectIntent",
                         "Trying to clear+get intent '%s' but '%s' is pending. Not clearing",
                         UserIntentTagToString( userIntent),
                         UserIntentTagToString( _pendingIntent->GetTag() ) );
    return nullptr;
  } else if( _pendingIntent != nullptr ) {
    return _pendingIntent.release();
  } else {
    PRINT_NAMED_WARNING( "UserIntentComponent.ClearUserIntentWithOwnership.NothingPending",
                         "Trying to clear+get intent '%s', but it doesn't exist",
                         UserIntentTagToString( userIntent ) );
    return nullptr;
  }
}
  
void UserIntentComponent::ClearUserIntentWithPreservation(UserIntentTag userIntent, BehaviorID clearingBehavior)
{
  if( (_pendingIntent != nullptr) && (userIntent != _pendingIntent->GetTag()) ) {
    PRINT_NAMED_WARNING( "UserIntentComponent.ClearUserIntentWithPreservation.IncorrectIntent",
                        "Trying to clear+get intent '%s' but '%s' is pending. Not clearing",
                        UserIntentTagToString( userIntent),
                        UserIntentTagToString( _pendingIntent->GetTag() ) );
  } else if( _pendingIntent != nullptr ) {
    auto it = std::find_if( _preservedIntents.begin(), _preservedIntents.end(), [userIntent](const auto& p) {
      return p.tag == userIntent;
    });
    
    if( it != _preservedIntents.end() ) {
      if( it->intent != nullptr ) {
        PRINT_NAMED_WARNING( "UserIntentComponent.ClearUserIntentWithPreservation.Replaced",
                             "A delegate of behavior '%s' never took the preserved intent '%s' before it was replaced by '%s'",
                             BehaviorTypesWrapper::BehaviorIDToString(it->responsibleBehavior),
                             UserIntentTagToString(userIntent),
                             BehaviorTypesWrapper::BehaviorIDToString(clearingBehavior) );
      }
      it->intent.reset( _pendingIntent.release() );
      it->responsibleBehavior = clearingBehavior;
    } else {
      std::unique_ptr<UserIntent> intent( _pendingIntent.release() );
      const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      Preserved preserved{userIntent, std::move(intent), clearingBehavior, currTime};
      _preservedIntents.emplace_back( std::move(preserved) );
    }
  } else {
    PRINT_NAMED_WARNING( "UserIntentComponent.ClearUserIntentWithPreservation.NothingPending",
                         "Trying to clear+get intent '%s', but it doesn't exist",
                         UserIntentTagToString( userIntent ) );
  }
}
  
UserIntent* UserIntentComponent::TakePreservedUserIntentOwnership(UserIntentTag userIntent)
{
  auto it = std::find_if( _preservedIntents.begin(), _preservedIntents.end(), [userIntent](const auto& p) {
    return p.tag == userIntent;
  });
  if( it != _preservedIntents.end() ) {
    UserIntent* intent = it->intent.release();
    ANKI_VERIFY( intent != nullptr,
                 "UserIntentComponent.TakePreservedUserIntentOwnership.AlreadyClaimed",
                 "Intent '%s' has been claimed already",
                 UserIntentTagToString(userIntent) );
    return intent;
  } else {
    PRINT_NAMED_WARNING( "UserIntentComponent.TakePreservedUserIntentOwnership.NotFound",
                         "Intent '%s' not found in preserved list",
                         UserIntentTagToString(userIntent) );
    return nullptr;
  }
}
  
void UserIntentComponent::ResetPreservedUserIntents(BehaviorID clearingBehavior)
{
  auto it = std::find_if( _preservedIntents.begin(), _preservedIntents.end(), [clearingBehavior](const auto& p) {
    return p.responsibleBehavior == clearingBehavior;
  });
  if( it != _preservedIntents.end() ) {
    if( !ANKI_VERIFY( it->intent == nullptr,
                      "UserIntentComponent.ResetPreservedUserIntents.Unclaimed",
                      "The intent '%s' preserved by '%s' was unclaimed",
                      UserIntentTagToString(it->tag),
                      BehaviorTypesWrapper::BehaviorIDToString(clearingBehavior) ) )
    {
      it->intent.reset();
    }
  }
}

void UserIntentComponent::SetUserIntentPending(UserIntentTag userIntent)
{
  // The following ensures that this method is only called for intents of type UserIntent_Void.
  // Ideally this would be a compile-time check, but that won't be possible unless all user intents
  // are hardcoded.
  // The uint8 below is a buffer of size 1, which matches the size of a UserIntent if it
  // has type UserIntent_Void. The first byte of the buffer is always the tag, so unpacking the
  // UserIntent from this buffer will succeed if the type is UserIntent_Void. If it fails, it will
  // assert in dev, but have no other repercussions.
  
  uint8_t buffer = Anki::Util::numeric_cast<uint8_t>( userIntent );
  UserIntent intent;
  intent.Unpack( &buffer, 1 ); // hit an assert? your userIntent is not of type UserIntent_Void. use overloaded method
  
  SetUserIntentPending( std::move(intent) );
  
  static_assert(std::is_same<std::underlying_type<UserIntentTag>::type, uint8_t>::value,
                "If you change type, the above needs revisiting");
}
  
void UserIntentComponent::SetUserIntentPending(UserIntent&& userIntent)
{
  if( _pendingIntent != nullptr ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetUserIntentPending.AlreadyPending",
                        "Setting pending user intent to '%s' which will overwrite '%s'",
                        UserIntentTagToString(userIntent.GetTag()),
                        UserIntentTagToString(_pendingIntent->GetTag()));
  }
  
  
  if( _pendingIntent == nullptr ) {
    _pendingIntent.reset( new UserIntent(userIntent) );
  } else {
    auto& intent = *_pendingIntent.get();
    intent = std::move(userIntent);
  }
  
  if( ANKI_DEV_CHEATS ) {
    SendWebVizIntents();
  }

  _pendingIntentTick = BaseStationTimer::getInstance()->GetTickCount();
}

void UserIntentComponent::SetCloudIntentPending(const std::string& cloudIntent)
{
  _devLastReceivedCloudIntent = cloudIntent;
  
  SetUserIntentPending( _intentMap->GetUserIntentFromCloudIntent(cloudIntent) );
}

bool UserIntentComponent::SetCloudIntentPendingFromJSON(const std::string& cloudJsonStr)
{
  // use the most permissive reader possible
  Json::Reader reader(Json::Features::all());
  Json::Value json;

  const bool parsedOK = reader.parse(cloudJsonStr, json, false);
  if( !parsedOK ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentPendingFromJSON.BadJson",
                        "Could not parse json from cloud string!");
    return false;
  }

  std::string cloudIntent;
  if( !JsonTools::GetValueOptional(json, kCloudIntentJsonKey, cloudIntent) ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentPendingFromJSON.MissingIntentKey",
                        "Cloud json missing key '%s'",
                        kCloudIntentJsonKey);
    return false;
  }
  
  auto& params = json[kParamsKey];
  const bool hasParams = !params.isNull();
  
  Json::Value emptyJson;
  Json::Value& intentJson = hasParams ? params : emptyJson;
  
  UserIntentTag userIntentTag = _intentMap->GetUserIntentFromCloudIntent(cloudIntent);
  
  if( hasParams ) {
    // translate variable names, if necessary
    _intentMap->SanitizeCloudIntentVariables( cloudIntent, params );
  }
  
  ANKI_VERIFY( json["type"].isNull(),
               "UserIntentComponent.SetCloudIntentPendingFromJson.Reserved",
               "cloud intent '%s' contains reserved key 'type'",
               cloudIntent.c_str() );

  UserIntent pendingIntent;
  
  // Set up json to look like a union
  intentJson["type"] = UserIntentTagToString(userIntentTag);
  const bool setOK = pendingIntent.SetFromJSON(intentJson);
  
  // the UserIntent will have size 1 if it's a UserIntent_Void, which means the user intent
  // corresponding to this cloud intent should _not_ have data.
  using Tag = std::underlying_type<UserIntentTag>::type;
  const bool expectedParams = (pendingIntent.Size() > sizeof(Tag));
  static_assert( std::is_same<Tag, uint8_t>::value,
                 "If the type changes, you need to rethink this");
    
  if( !setOK ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentPendingFromJSON.BadParams",
                        "could not parse user intent '%s' from cloud intent of type '%s'",
                        UserIntentTagToString(userIntentTag),
                        cloudIntent.c_str());
    // NOTE: also don't set the pending intent, since the request was malformed
    return false;
  } else if( !expectedParams && hasParams ) {
    // simply ignore the extraneous data but continue
    PRINT_NAMED_WARNING( "UserIntentComponent.SetCloudIntentPendingFromJson.ExtraData",
                         "Intent '%s' has unexpected params",
                         cloudIntent.c_str() );
  } else if( expectedParams && !hasParams ) {
    // missing params, bail
    PRINT_NAMED_WARNING( "UserIntentComponent.SetCloudIntentPendingFromJson.MissingParams",
                         "Intent '%s' did not contain required params",
                         cloudIntent.c_str() );
    return false;
  }
  
  _devLastReceivedCloudIntent = cloudIntent;
  
  SetUserIntentPending( std::move(pendingIntent) );
  
  return true;
}

void UserIntentComponent::Update()
{
  
  {
    std::lock_guard<std::mutex> lock{_mutex};
    if( !_pendingCloudIntent.empty() ) {
      SetCloudIntentPendingFromJSON( _pendingCloudIntent );
      _pendingCloudIntent.clear();
    }
  }
  
  
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // if things pend too long they will queue up and trigger at the wrong time, which will be wrong and
  // confusing. Issue warnings here and clear the pending tick / intent if they aren't handled quickly enough
  
  if( _pendingTrigger ) {
    const size_t dt = currTick - _pendingTriggerTick;
    if( dt >= kMaxTicksToWarn ) {
      PRINT_NAMED_WARNING("UserIntentComponent.Update.PendingTriggerNotCleared",
                          "Trigger has been pending for %zu ticks",
                          dt);
    }
    if( dt >= kMaxTicksToClear ) {      
      PRINT_NAMED_ERROR("UserIntentComponent.Update.PendingTriggerNotCleared.ForceClear",
                        "Trigger has been pending for %zu ticks, forcing a clear",
                        dt);
      _pendingTrigger = false;
    }
  }

  if( _pendingIntent != nullptr ) {
    const size_t dt = currTick - _pendingIntentTick;
    if( dt >= kMaxTicksToWarn ) {
      PRINT_NAMED_WARNING("UserIntentComponent.Update.PendingIntentNotCleared.Warn",
                          "Intent '%s' has been pending for %zu ticks",
                          UserIntentTagToString(_pendingIntent->GetTag()),
                          dt);
    }
    if( dt >= kMaxTicksToClear ) {      
      PRINT_NAMED_ERROR("UserIntentComponent.Update.PendingIntentNotCleared.ForceClear",
                        "Intent '%s' has been pending for %zu ticks, forcing a clear",
                        UserIntentTagToString(_pendingIntent->GetTag()),
                        dt);
      _pendingIntent.reset();
      _wasIntentUnclaimed = true;
    }
  }
  
  for( auto& preserved : _preservedIntents ) {
    if( preserved.intent != nullptr ) {
      if( currTime - preserved.timeAdded > kPreservedTimeToClear ) {
        PRINT_NAMED_ERROR( "UserIntentComponent.Update.UnclaimedPreserved",
                           "The preserved intent '%s' was unclaimed",
                           UserIntentTagToString(preserved.tag) );
        preserved.intent.reset();
      }
    }
  }
}
  
void UserIntentComponent::OnCloudData(std::string&& data)
{
  PRINT_CH_INFO( "BehaviorSystem", "UserIntentComponent.OnCloudData", "'%s'", data.c_str() );
  
  std::lock_guard<std::mutex> lock{_mutex};
  _pendingCloudIntent = data;
}
  
void UserIntentComponent::OnAppIntent(const ExternalInterface::AppIntent& appIntent )
{
  UserIntentTag userIntentTag = _intentMap->GetUserIntentFromAppIntent( appIntent.intent );
  
  Json::Value json;
  // todo: eventually AppIntent should be its own union of structures, but
  // currently there's only one intent, with one arg, and it's not possible to transmit
  // a union over the temporary webservice handler. Once the real app-engine channels are open,
  // these two lines will need replacing
  json["type"] = UserIntentTagToString(userIntentTag);
  json["param"] = appIntent.param;
  
  _intentMap->SanitizeAppIntentVariables( appIntent.intent, json );
  
  UserIntent intent;
  if( ANKI_VERIFY( intent.SetFromJSON(json),
                   "UserIntentComponent.OnAppIntent.BadJson",
                   "Could not create user intent from app intent '%s'",
                   appIntent.intent.c_str() ) )
  {
    _devLastReceivedAppIntent = appIntent.intent;
    
    SetUserIntentPending( std::move(intent) );
  }
}
  
std::string UserIntentComponent::GetServerName(const Robot& robot) const
{
  return "ai_sock" + ((robot.GetID() == 0)
                      ? ""
                      : std::to_string(robot.GetID()));  // Offset port by robotID so that we can run sims with multiple robots
}
  
std::vector<std::string> UserIntentComponent::DevGetCloudIntentsList() const
{
  return _intentMap->DevGetCloudIntentsList();
}
  
std::vector<std::string> UserIntentComponent::DevGetAppIntentsList() const
{
  return _intentMap->DevGetAppIntentsList();
}
  
void UserIntentComponent::SendWebVizIntents()
{
  // if( _context != nullptr ) {
  //   const auto* webService = _context->GetWebService();
    
  //   Json::Value toSend = Json::arrayValue;
    
  //   {
  //     Json::Value blob;
  //     blob["intentType"] = "user";
  //     blob["type"] = "current-intent";
  //     blob["value"] = UserIntentTagToString( _pendingIntent->GetTag() );
  //     toSend.append(blob);
  //   }
    
  //   if( !_devLastReceivedCloudIntent.empty() ) {
  //     Json::Value blob;
  //     blob["intentType"] = "cloud";
  //     blob["type"] = "current-intent";
  //     blob["value"] = _devLastReceivedCloudIntent;
  //     toSend.append(blob);
  //     _devLastReceivedCloudIntent.clear();
  //   }
    
  //   if( !_devLastReceivedAppIntent.empty() ) {
  //     Json::Value blob;
  //     blob["intentType"] = "app";
  //     blob["type"] = "current-intent";
  //     blob["value"] = _devLastReceivedAppIntent;
  //     toSend.append(blob);
  //     _devLastReceivedAppIntent.clear();
  //   }
    
  //   // webService->SendToWebViz( "intents", toSend );
  // }
}

}
}
