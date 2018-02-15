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

#include "engine/aiComponent/userIntentComponent.h"

#include "engine/aiComponent/userIntentMap.h"

#include "clad/types/userIntents.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"


#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace {

static const size_t kMaxTicksToWarn = 1;
static const size_t kMaxTicksToClear = 2;

static const char* kCloudIntentJsonKey = "intent";
}

UserIntentComponent::UserIntentComponent(const Json::Value& userIntentMapConfig)
  : _intentMap(new UserIntentMap(userIntentMapConfig))
  , _pendingExtraData(new UserIntentData)
{
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

bool UserIntentComponent::IsValidUserIntent(const std::string& userIntent) const
{
  DEV_ASSERT(_intentMap, "UserIntentComponent.NoIntentMap");
  return _intentMap->IsValidUserIntent(userIntent);
}

bool UserIntentComponent::IsAnyUserIntentPending() const
{
  return !_pendingIntent.empty();
}

bool UserIntentComponent::IsUserIntentPending(const std::string& userIntent) const
{
  DEV_ASSERT_MSG( IsValidUserIntent(userIntent),
                  "UserIntentComponent.IsUserIntentPending.InvalidIntent",
                  "Asking if '%s' is pending, but that's not a valid intent",
                  userIntent.c_str());
  
  return _pendingIntent == userIntent;
}


bool UserIntentComponent::IsUserIntentPending(const std::string& userIntent, UserIntentData& extraData) const
{
  if( IsUserIntentPending(userIntent) ) {
    extraData = *_pendingExtraData;
    return true;
  }
  else {
    return false;
  }
}

void UserIntentComponent::ClearUserIntent(const std::string& userIntent)
{
  DEV_ASSERT_MSG( IsValidUserIntent(userIntent),
                  "UserIntentComponent.ClearUserIntent.InvalidIntent",
                  "Trying to clear '%s', but that's not a valid intent",
                  userIntent.c_str());

  if( userIntent != _pendingIntent ) {
    PRINT_NAMED_WARNING("UserIntentComponent.ClearUserIntent.IncorrectIntent",
                        "Trying to clear intent '%s' but '%s' is pending. Not clearing",
                        userIntent.c_str(),
                        _pendingIntent.c_str());
  }
  else {
    _pendingIntent.clear();
    _pendingExtraData->Set_none({});
  }
}

void UserIntentComponent::SetUserIntentPending(const std::string& userIntent)
{
  DEV_ASSERT_MSG( IsValidUserIntent(userIntent),
                  "UserIntentComponent.ClearUserIntent.InvalidIntent",
                  "Trying to clear '%s', but that's not a valid intent",
                  userIntent.c_str());

  if( !_pendingIntent.empty() ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetPendingIntent.AlreadyPending",
                        "Setting pending user intent to '%s' which will overwrite '%s'",
                        userIntent.c_str(),
                        _pendingIntent.c_str());
  }
  _pendingIntent = userIntent;

  if( _pendingIntent.empty() ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetPendingIntent.EmptyIntent",
                        "Trying to set an empty string as the intent is invalid. Use ClearUserIntent instead");
  }
  else {
    _pendingIntentTick = BaseStationTimer::getInstance()->GetTickCount();
  }

}

void UserIntentComponent::SetCloudIntentPending(const std::string& cloudIntent)
{
  SetUserIntentPending( _intentMap->GetUserIntentFromCloudIntent(cloudIntent) );
}

bool UserIntentComponent::SetCloudIntentFromJSON(const std::string& cloudJsonStr)
{
  // use the most permissive reader possible
  Json::Reader reader(Json::Features::all());
  Json::Value json;

  const bool parsedOK = reader.parse(cloudJsonStr, json, false);
  if( !parsedOK ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentFromJSON.BadJson",
                        "Could not parse json from cloud string!");
    return false;
  }

  std::string cloudIntent;
  if( !JsonTools::GetValueOptional(json, kCloudIntentJsonKey, cloudIntent) ) {
    PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentFromJSON.MissingIntentKey",
                        "Cloud json missing key '%s'",
                        kCloudIntentJsonKey);
    return false;
  }

  const std::string& extraDataType = _intentMap->GetCloudIntentExtraData(cloudIntent);
  const bool hasExtraData = !extraDataType.empty();

  if( hasExtraData ) {

    // there is extra data present. Set up json to look like a union
    json["type"] = extraDataType;
    const bool setOK = _pendingExtraData->SetFromJSON(json);
    if( !setOK ) {
      PRINT_NAMED_WARNING("UserIntentComponent.SetCloudIntentFromJSON.BadExtraData",
                          "could not parse extra data of type '%s' from cloud intent of type '%s'",
                          extraDataType.c_str(),
                          cloudIntent.c_str());
      // NOTE: also don't set the pending intent, since the request was malformed
      return false;
    }
  }

  SetCloudIntentPending(cloudIntent);

  return true;
}

void UserIntentComponent::Update()
{
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();

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

  if( !_pendingIntent.empty() ) {
    const size_t dt = currTick - _pendingIntentTick;
    if( dt >= kMaxTicksToWarn ) {
      PRINT_NAMED_WARNING("UserIntentComponent.Update.PendingIntentNotCleared.Warn",
                          "Intent '%s' has been pending for %zu ticks",
                          _pendingIntent.c_str(),
                          dt);
    }
    if( dt >= kMaxTicksToClear ) {      
      PRINT_NAMED_ERROR("UserIntentComponent.Update.PendingIntentNotCleared.ForceClear",
                        "Intent '%s' has been pending for %zu ticks, forcing a clear",
                        _pendingIntent.c_str(),
                        dt);
      _pendingIntent.clear();
    }
  }
}

}
}
