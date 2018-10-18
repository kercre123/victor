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
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/userIntentMap.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/animationComponent.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/behaviorTriggerResponse.h"
#include "clad/types/behaviorComponent/userIntent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleFunction.h"
#include "util/logging/DAS.h"
#include "webServerProcess/src/webVizSender.h"


#include "json/json.h"

#define LOG_CHANNEL "BehaviorSystem"

namespace Anki {
namespace Vector {

namespace {

static const size_t kMaxTicksToWarn = 2;
static const size_t kMaxTicksToClear = 3;
static const float kTimeToClearWaitingForTriggerWordGetIn_s = 3.0f;
static const char* kCloudIntentJsonKey = "intent";
static const char* kParamsKey = "params";
static const char* kAltParamsKey = "parameters"; // "params" is reserved in CLAD

  CONSOLE_VAR(bool, kStreamAfterDevWakeWord, "UserIntentComponent", false);
  CONSOLE_VAR(bool, kPlayGetInAfterDevWakeWord, "UserIntentComponent", false);

}

size_t UserIntentComponent::sActivatedIntentID = 0;

const UserIntentSource& GetIntentSource(const UserIntentData& intentData)
{
  return intentData.source;
}

UserIntentComponent::UserIntentComponent(const Robot& robot, const Json::Value& userIntentMapConfig)
  : IDependencyManagedComponent( this, BCComponentID::UserIntentComponent )
  , _intentMap( new UserIntentMap(userIntentMapConfig, robot.GetContext()) )
  , _context( robot.GetContext() )
{

  // setup cloud intent handler
  const auto& serverName = GetServerName( robot );
  _server.reset( new BehaviorComponentCloudServer( _context, std::bind( &UserIntentComponent::OnCloudData,
                                                                        this,
                                                                        std::placeholders::_1),
                                                                        serverName ) );

  // setup trigger word handler
  auto triggerWordCallback = [this]( const AnkiEvent<RobotInterface::RobotToEngine>& event ){
    const bool willStream = event.GetData().Get_triggerWordDetected().willOpenStream;
    SetTriggerWordPending(willStream);

    HandleTriggerWordEventForDas(event.GetData().Get_triggerWordDetected());
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

  SetupConsoleFuncs();

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
    LOG_WARNING("UserIntentComponent.ClearPendingTrigger.TriggerNotSet",
                "Trying to clear trigger but the trigger isn't set. This is likely a bug");
  }
  else {
    _pendingTrigger = false;
  }
}

void UserIntentComponent::SetTriggerWordPending(const bool willOpenStream)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // assume get in animation is playing
  _waitingForTriggerWordGetInToFinish = true;
  _waitingForTriggerWordGetInToFinish_setTime_s = currTime_s;

  if(!_responseToTriggerWordMap.empty()){
    // TODO: VIC-5733 This also needs to either check if _responseToTriggerWordMap contains an empty anim, or listen
    // for playing anims and compare tags
    auto lastElemIter = _responseToTriggerWordMap.rbegin();
    _robot->GetAnimationComponent().NotifyComponentOfAnimationStartedByAnimProcess(
      lastElemIter->response.getInAnimationName, lastElemIter->response.getInAnimationTag);
  }
  if (!GetEngineShouldRespondToTriggerWord()) {
    LOG_DEBUG("UserIntentComponent.SetPendingTrigger.TriggerWordDetectionDisabled",
              "Trigger word detection disabled, so ignoring message");
    return;
  }

  if (_pendingTrigger) {
    LOG_WARNING("UserIntentComponent.SetPendingTrigger.AlreadyPending",
                "setting a pending trigger word but the last one hasn't been cleared");
  }

  _pendingTrigger = true;
  _pendingTriggerWillStream = willOpenStream;
  _pendingTriggerTick = BaseStationTimer::getInstance()->GetTickCount();

  LOG_INFO("UserIntentComponent.SetTriggerWordPending",
           "Set trigger word pending (%s stream) at tick %zu",
           willOpenStream ? "will" : "will not",
           _pendingTriggerTick );

  if (_wasIntentError) {
    LOG_WARNING("UserIntentComponent.SetTriggerWordPending.ClearingError",
                "Previous intent gave us an error, but a new trigger word came in. Clearing the old error");
    _wasIntentError = false;
  }
}

bool UserIntentComponent::IsAnyUserIntentPending() const
{
  return (_pendingIntent != nullptr);
}

bool UserIntentComponent::IsUserIntentPending(UserIntentTag userIntent) const
{
  return (_pendingIntent != nullptr) && (_pendingIntent->intent.GetTag() == userIntent);
}

UserIntentPtr UserIntentComponent::ActivateUserIntent(UserIntentTag userIntent, const std::string& owner, bool showFeedback)
{
  if (!IsUserIntentPending(userIntent)) {
    LOG_ERROR("UserIntentComponent.ActivateIntent.NoActive",
              "'%s' is attempting to activate intent '%s', but %s is pending",
              owner.c_str(),
              UserIntentTagToString(userIntent),
              _pendingIntent ? UserIntentTagToString(_pendingIntent->intent.GetTag()) : "nothing");
    return nullptr;
  }

  if (_activeIntent != nullptr) {
    LOG_WARNING("UserIntentComponent.ActivateIntent.IntentAlreadyActive",
                "%s is Trying to activate user intent '%s', but '%s' is still active",
                owner.c_str(),
                UserIntentTagToString(userIntent),
                UserIntentTagToString(_activeIntent->intent.GetTag()));
  }

  LOG_DEBUG("UserIntentComponent.ActivateUserIntent",
            "%s is activating intent '%s'",
            owner.c_str(),
            UserIntentTagToString(userIntent));

  _activeIntent = std::move(_pendingIntent);
  _activeIntent->activationID = ++sActivatedIntentID;

  // track the owner for easier debugging
  _activeIntentOwner = owner;

  if( showFeedback ){
    _activeIntentFeedback.Activate(userIntent);
  }

  return _activeIntent;
}

void UserIntentComponent::DeactivateUserIntent(UserIntentTag userIntent)
{
  // we can have nested activate calls, and we want to be able to deactivate "older" activated intents
  if (userIntent != UserIntentTag::INVALID) {
    _activeIntentFeedback.Deactivate(userIntent);
  }
    
  if (!IsUserIntentActive(userIntent)) {
    LOG_ERROR("UserIntentComponent.DeactivateUserIntent.NotActive",
              "Attempting to deactivate intent '%s' (activated by %s) but '%s' is active",
              UserIntentTagToString(userIntent),
              _activeIntentOwner.c_str(),
              _activeIntent ? UserIntentTagToString(_activeIntent->intent.GetTag()) : "nothing");
    return;
  }

  LOG_DEBUG("UserIntentComponent.DeactivateUserIntent",
            "Deactivating intent '%s' (activated by %s)",
            UserIntentTagToString(userIntent),
            _activeIntentOwner.c_str());
  _activeIntent.reset();
  _activeIntentOwner.clear();

}

void UserIntentComponent::StopActiveUserIntentFeedback()
{
  _activeIntentFeedback.Deactivate(UserIntentTag::INVALID);
}

// todo: remove this when we decide what we're doing with the lights
#define USE_CUSTOM_BP_ANIM 0

UserIntentComponent::ActiveIntentFeedback::ActiveIntentFeedback() :
  robot(nullptr),
  activatedIntentTag(UserIntentTag::INVALID)
{

}

void UserIntentComponent::ActiveIntentFeedback::Init(Robot* robot)
{
  this->robot = robot;
  DEV_ASSERT( this->robot != nullptr, "UserIntentComponent.ActiveIntentFeedback.Init: Invalid Robot pointer" );
}

void UserIntentComponent::ActiveIntentFeedback::Activate(UserIntentTag userIntent)
{
  if (!IsEnabled()) {
    return;
  }

  // don't activate if we're currently actiavted
  // some behavior have nested "intent activations" and we only care about the first one
  if (!IsActive())
  {
    #if USE_CUSTOM_BP_ANIM
    {
      static const BackpackLightAnimation::BackpackAnimation kActiveStateLights =
      {
        .onColors               = {{NamedColors::WHITE,NamedColors::WHITE,NamedColors::WHITE}},
        .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
        .onPeriod_ms            = {{1000,1000,1000}},
        .offPeriod_ms           = {{250,250,250}},
        .transitionOnPeriod_ms  = {{500,500,500}},
        .transitionOffPeriod_ms = {{500,500,500}},
        .offset                 = {{0,0,0}}
      };

      BackpackLightComponent& bplComponent = robot->GetBackpackLightComponent();
      bplComponent.StartLoopingBackpackAnimation( kActiveStateLights, lightsHandle );
    }
    #else
    {
      BackpackLightComponent& bplComponent = robot->GetBackpackLightComponent();
      bplComponent.SetBackpackAnimation( BackpackAnimationTrigger::MeetVictor );
    }
    #endif

    // todo: send "start" audio event

    // store who activated the state so only it may deactivate it
    activatedIntentTag = userIntent;
  }
}

void UserIntentComponent::ActiveIntentFeedback::Deactivate(UserIntentTag userIntent)
{
  if (!IsEnabled()) {
    return;
  }

  // only deactivate if a) we're currently active, and b) this is the same intent that activated us
  // UserIntentTag::INVALID intent will force deactivation
  if (IsActive() && ((userIntent == activatedIntentTag) || (userIntent == UserIntentTag::INVALID)))
  {
    #if USE_CUSTOM_BP_ANIM
    {
      if ( lightsHandle.IsValid() )
      {
        BackpackLightComponent& bplComponent = robot->GetBackpackLightComponent();
        bplComponent.StopLoopingBackpackAnimation( lightsHandle );
      }
    }
    #else
    {
      BackpackLightComponent& bplComponent = robot->GetBackpackLightComponent();
      bplComponent.ClearAllBackpackLightConfigs();
    }
    #endif

    // todo: send "stop" audio event

    activatedIntentTag = UserIntentTag::INVALID;
  }
}

bool UserIntentComponent::ActiveIntentFeedback::IsEnabled() const
{
  return ( (nullptr != robot)
        && robot->GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::ActiveIntentFeedback) );
}

bool UserIntentComponent::ActiveIntentFeedback::IsActive() const
{
  return activatedIntentTag != UserIntentTag::INVALID;
}

bool UserIntentComponent::IsUserIntentActive(UserIntentTag userIntent) const
{
  return ( _activeIntent != nullptr ) && ( _activeIntent->intent.GetTag() == userIntent );
}

UserIntentPtr UserIntentComponent::GetUserIntentIfActive(UserIntentTag forIntent) const
{
  auto ret = IsUserIntentActive(forIntent) ? _activeIntent : nullptr;
  return ret;
}

void UserIntentComponent::DropUserIntent(UserIntentTag userIntent)
{
  if (IsUserIntentPending(userIntent)) {
    _pendingIntent.reset();
  }

  LOG_WARNING("UserIntentComponent.DropUserIntent.NotPending",
              "Trying to drop intent '%s' but %s is pending",
              UserIntentTagToString(userIntent),
              _pendingIntent ? UserIntentTagToString(_pendingIntent->intent.GetTag()) : "nothing");

}

void UserIntentComponent::DropAnyUserIntent()
{
  if (!IsAnyUserIntentPending()) {
    LOG_WARNING("UserIntentComponent.DropAnyUserIntent.IntentNotSet",
                "Trying to clear a pending intent but the intent isn't set. This is likely a bug");
  }
  _pendingIntent.reset();
}

bool UserIntentComponent::IsUserIntentPending(UserIntentTag userIntent, UserIntent& extraData) const
{
  if (IsUserIntentPending(userIntent)) {
    extraData = _pendingIntent->intent;
    return true;
  }
  return false;
}

void UserIntentComponent::SetUserIntentPending(UserIntentTag userIntent, const UserIntentSource& source)
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

  SetUserIntentPending( std::move(intent), source );

  static_assert(std::is_same<std::underlying_type<UserIntentTag>::type, uint8_t>::value,
                "If you change type, the above needs revisiting");
}

void UserIntentComponent::SetUserIntentPending(UserIntent&& userIntent, const UserIntentSource& source)
{
  if (_pendingIntent != nullptr) {
    LOG_WARNING("UserIntentComponent.SetUserIntentPending.AlreadyPending",
                "Setting pending user intent to '%s' which will overwrite '%s'",
                UserIntentTagToString(userIntent.GetTag()),
                UserIntentTagToString(_pendingIntent->intent.GetTag()));
  }


  if (_pendingIntent == nullptr) {
    _pendingIntent.reset( new UserIntentData(userIntent, source) );
  } else {
    _pendingIntent->intent = std::move(userIntent);
    _pendingIntent->source = source;
  }

  if (ANKI_DEV_CHEATS) {
    SendWebVizIntents();
  }

  _pendingIntentTick = BaseStationTimer::getInstance()->GetTickCount();
  _pendingIntentTimeoutEnabled = true;
}

void UserIntentComponent::DevSetUserIntentPending(UserIntentTag userIntent, const UserIntentSource& source)
{
  SetUserIntentPending(userIntent, source);
}

void UserIntentComponent::DevSetUserIntentPending(UserIntent&& userIntent, const UserIntentSource& source)
{
  SetUserIntentPending(std::move(userIntent), source);
}

void UserIntentComponent::DevSetUserIntentPending(UserIntentTag userIntent)
{
  SetUserIntentPending(userIntent, UserIntentSource::Unknown);
}

void UserIntentComponent::DevSetUserIntentPending(UserIntent&& userIntent)
{
  SetUserIntentPending(std::move(userIntent), UserIntentSource::Unknown);
}

void UserIntentComponent::SetUserIntentTimeoutEnabled(bool isEnabled)
{
  // if we're re-enabling the timeout warning, reset the tick count
  if (isEnabled && !_pendingIntentTimeoutEnabled) {
    _pendingIntentTick = BaseStationTimer::getInstance()->GetTickCount();
  }
  _pendingIntentTimeoutEnabled = isEnabled;
}

bool UserIntentComponent::SetCloudIntentPendingFromExpandedJSON(const std::string& cloudJsonStr)
{
  // use the most permissive reader possible
  Json::Reader reader(Json::Features::all());
  Json::Value json;

  const bool parsedOK = reader.parse(cloudJsonStr, json, false);
  if (!parsedOK) {
    LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromExpandedJSON.BadJson",
                "Could not parse json from cloud string!");
    return false;
  }

  return SetIntentPendingFromCloudJSONValue(std::move(json));
}
bool UserIntentComponent::SetIntentPendingFromCloudJSONValue(Json::Value json)
{
  std::string cloudIntent;
  if (!JsonTools::GetValueOptional(json, kCloudIntentJsonKey, cloudIntent)) {
    LOG_WARNING("UserIntentComponent.SetIntentPendingFromCloudJSONValue.MissingIntentKey",
                "Cloud json missing key '%s'",
                 kCloudIntentJsonKey);
    return false;
  }

  auto& params = json[kParamsKey];
  const bool hasParams = !params.isNull();

  Json::Value emptyJson;
  Json::Value& intentJson = hasParams ? params : emptyJson;

  UserIntentTag userIntentTag = _intentMap->GetUserIntentFromCloudIntent(cloudIntent);

  if (hasParams) {
    // translate variable names, if necessary
    _intentMap->SanitizeCloudIntentVariables( cloudIntent, params );
  }

  ANKI_VERIFY( json["type"].isNull(),
               "UserIntentComponent.SetIntentPendingFromCloudJSONValue.Reserved",
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

  if (!setOK) {
    LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromJSON.BadParams",
                "could not parse user intent '%s' from cloud intent of type '%s'",
                UserIntentTagToString(userIntentTag),
                cloudIntent.c_str());
    // NOTE: also don't set the pending intent, since the request was malformed
    return false;
  } else if (!expectedParams && hasParams) {
    // simply ignore the extraneous data but continue
    LOG_WARNING("UserIntentComponent.SetIntentPendingFromCloudJSONValue.ExtraData",
                "Intent '%s' has unexpected params",
                cloudIntent.c_str() );
  } else if (expectedParams && !hasParams) {
    // missing params, bail
    LOG_WARNING("UserIntentComponent.SetIntentPendingFromCloudJSONValue.MissingParams",
                "Intent '%s' did not contain required params",
                cloudIntent.c_str() );
    return false;
  }


  if (!_whitelistedIntents.empty()) {
    // only pass on whitelisted intents
    if (_whitelistedIntents.find(userIntentTag) == _whitelistedIntents.end()) {
      LOG_INFO("UserIntentComponent.IgnoringNonWhitelist.Cloud", "Ignoring intent %s", UserIntentTagToString(userIntentTag));
      pendingIntent = UserIntent::Createunmatched_intent({});
    }
  }

  _devLastReceivedCloudIntent = cloudIntent;

  DevSetUserIntentPending( std::move(pendingIntent), UserIntentSource::Voice );

  return true;
}

void UserIntentComponent::InitDependent( Vector::Robot* robot, const BCCompMap& dependentComps )
{
  _robot = robot;

  auto callback = [this](){
    _waitingForTriggerWordGetInToFinish = false;
  };

  _tagForTriggerWordGetInCallbacks = _robot->GetAnimationComponent().SetTriggerWordGetInCallback(callback);

  _activeIntentFeedback.Init(robot);
}

bool UserIntentComponent::SetCloudIntentPendingFromString(const std::string& cloudStr)
{
  std::lock_guard<std::mutex> lock{_mutex};

  if (_pendingCloudIntent.GetTag() != CloudMic::MessageTag::INVALID) {
    LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromString.AlreadyPending",
                "A cloud intent was already pending and will be overwritten");
  }

  Json::Reader reader(Json::Features::all());
  Json::Value cloudJSON;
  const bool stringOK = reader.parse( cloudStr, cloudJSON, true );

  if (!stringOK) {
    LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromString.InvalidString",
                "Could not convert string to json '%s'",
                cloudStr.c_str());
    return false;
  }

  if (cloudJSON["type"].isNull()) {
    LOG_DEBUG("UserIntentComponent.SetCloudIntentPendingFromString.SetResultType",
             "No specified type for the cloud message, assuming 'result'");
    cloudJSON["type"] = CloudMic::MessageTagToString(CloudMic::MessageTag::result);
  }

  const bool parseOK = _pendingCloudIntent.SetFromJSON( cloudJSON );

  if (!parseOK) {
    LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromString.InvalidJSON",
                "Could not convert (valid) JSON to clad object '%s'",
                cloudStr.c_str());
    return false;
  }

  // unfortunately, CLAD SetFromJSON is way _too_ permissive, so let's make sure we actually got a reasonable
  // value
  switch (_pendingCloudIntent.GetTag()) {
    case CloudMic::MessageTag::INVALID: {
      LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromString.InvalidMessage",
                  "Converted, but didn't have valid tag type: '%s'",
                  cloudStr.c_str());
      _pendingCloudIntent = {};
      return false;
    }

    case CloudMic::MessageTag::result: {
      if (_pendingCloudIntent.Get_result().intent.empty()) {
        LOG_WARNING("UserIntentComponent.SetCloudIntentPendingFromString.InvalidResult",
                    "Parsed cloud result message, but have no intent: '%s'",
                    cloudStr.c_str());
        _pendingCloudIntent = {};
        return false;
      }
      else {
        // intent OK (or at least is a valid string, which is all we check for now)
        break;
      }
    }

    default: break;  // TODO: add more detailed checks for other types of messages
  }

  // if we get here, message seems OK

  Json::FastWriter writer __attribute__((unused));

  // convert back to string and print so user can verify everything was set as expected
  LOG_DEBUG("UserIntentComponent.SetCloudIntentPendingFromString.Set",
            "Successfully set pending intent: '%s'",
            writer.write(_pendingCloudIntent.GetJSON()).c_str());
  return true;
}

void UserIntentComponent::UpdateDependent(const BCCompMap& dependentComps)
{
  {
    std::lock_guard<std::mutex> lock{_mutex};
    if (_pendingCloudIntent.GetTag() != CloudMic::MessageTag::INVALID) {
      switch ( _pendingCloudIntent.GetTag() ) {
        case CloudMic::MessageTag::result:
        {
          auto json = _pendingCloudIntent.Get_result().GetJSON();
          bool ok = true;
          if ( json[kAltParamsKey].isString() ) {
            // params are encoded as a string, but we need to make it a Json::Value
            std::string jsonStr{ json[kAltParamsKey].asString() };
            if ( jsonStr.size() > 0 ) {
              Json::Reader reader;
              Json::Value val;
              ok = reader.parse( jsonStr, val, false );
              if (!ok) {
                LOG_WARNING("UserIntentComponent.UpdatePendingIntent.BadJson",
                            "Could not parse json from cloud string: %s", jsonStr.c_str());
              }
              else if ( val.size() > 0 ) {
                json[kParamsKey] = std::move(val);
              }
            }
            json.removeMember(kAltParamsKey);
          }
          if ( ok ) {
            SetIntentPendingFromCloudJSONValue( std::move( json ) );
          }
          _isStreamOpen = false;
          _pendingTriggerWillStream = false;

          if (_wasIntentError) {
            LOG_WARNING("UserIntentComponent.GotCloudIntent.ClearingError",
                        "Previous intent gave us an error, but a new intent word came in. Clearing the error");
            _wasIntentError = false;
          }

          break;
        }

        case CloudMic::MessageTag::streamTimeout:
        case CloudMic::MessageTag::error:
        {
          LOG_WARNING("UserIntentComponent.UpdatePendingIntent.GotError",
                      "Got cloud error message type %s",
                      CloudMic::MessageTagToString( _pendingCloudIntent.GetTag()));

          {
            DASMSG( robot_cloud_response_failed, "robot.cloud_response_failed", "Invalid response received from the cloud" );
            DASMSG_SET( s1, ErrorTypeToString( _pendingCloudIntent.Get_error().error ), "The error string" );
            DASMSG_SET( i1, (int)(_pendingCloudIntent.GetTag() == CloudMic::MessageTag::error), "Whether it was a timeout (0) or error (1) " );
            DASMSG_SEND();
          }

          _wasIntentError = true;
          _isStreamOpen = false;
          _pendingTriggerWillStream = false;
          break;
        }

        case CloudMic::MessageTag::streamOpen:
        {
          LOG_DEBUG("UserIntentComponent.UpdatePendingIntent.StreamOpen", "Now streaming to cloud");
          _isStreamOpen = true;
          break;
        }

        default:
          LOG_WARNING("UserIntentComponent.UpdatePendingIntent.SkipOther",
                      "Skipping non-intent (and non-error) result cloud message: '%s'",
                      CloudMic::MessageTagToString( _pendingCloudIntent.GetTag() ) );
      }
      _pendingCloudIntent = {};
    }
  }


  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // if things pend too long they will queue up and trigger at the wrong time, which will be wrong and
  // confusing. Issue warnings here and clear the pending tick / intent if they aren't handled quickly enough

  if (_pendingTrigger) {
    const size_t dt = currTick - _pendingTriggerTick;
    if (dt >= kMaxTicksToWarn) {
      LOG_WARNING("UserIntentComponent.Update.PendingTriggerNotCleared",
                  "Trigger has been pending for %zu ticks",
                   dt);
    }
    if (dt >= kMaxTicksToClear) {
      LOG_ERROR("UserIntentComponent.Update.PendingTriggerNotCleared.ForceClear",
                "Trigger has been pending for %zu ticks, forcing a clear",
                 dt);
      _pendingTrigger = false;
      _pendingTriggerWillStream = false;
      _waitingForTriggerWordGetInToFinish = false;

      DASMSG(trigger_dropped, "behavior.trigger_word.dropped", "Engine got a trigger word, but no behavior claimed it");
      DASMSG_SEND_WARNING();

    }
  }

  if (_waitingForTriggerWordGetInToFinish) {
    const float dt = currTime_s - _waitingForTriggerWordGetInToFinish_setTime_s;
    if (dt >= kTimeToClearWaitingForTriggerWordGetIn_s) {
      LOG_WARNING("UserIntentComponent.Update.WaitingForTriggerWordGetInToFinish.ForceClear",
                  "Have been waiting for %f seconds for trigger word get in anim to finish, going ahead anyway",
                  dt);
      _waitingForTriggerWordGetInToFinish = false;
    }
  }


  if( _pendingIntent != nullptr ) {
    if( _pendingIntentTimeoutEnabled ) {
      const size_t dt = currTick - _pendingIntentTick;
      if( dt >= kMaxTicksToWarn ) {
        LOG_WARNING("UserIntentComponent.Update.PendingIntentNotCleared.Warn",
                    "Intent '%s' has been pending for %zu ticks",
                    UserIntentTagToString(_pendingIntent->intent.GetTag()),
                    dt);
      }
      if( dt >= kMaxTicksToClear ) {
        LOG_ERROR("UserIntentComponent.Update.PendingIntentNotCleared.ForceClear",
                  "Intent '%s' has been pending for %zu ticks, forcing a clear",
                  UserIntentTagToString(_pendingIntent->intent.GetTag()),
                  dt);

        DASMSG(intent_dropped, "behavior.voice_command.dropped",
               "Engine got a user intent (e.g. voice command), but no behavior activated it");
        DASMSG_SET(s1, UserIntentTagToString( _pendingIntent->intent.GetTag() ), "the intent type that was dropped");
        DASMSG_SET(s2, UserIntentSourceToString( _pendingIntent->source ),
                   "Source of the intent that was dropped (e.g. App, Voice)");
        DASMSG_SEND_WARNING();

        _pendingIntent.reset();
        _wasIntentUnclaimed = true;

      }
    }
  }
}


void UserIntentComponent::StartWakeWordlessStreaming( CloudMic::StreamType streamType, bool playGetInFromAnimProcess )
{
  RobotInterface::StartWakeWordlessStreaming message{ static_cast<uint8_t>(streamType), playGetInFromAnimProcess};
  _robot->SendMessage( RobotInterface::EngineToRobot( std::move(message) ) );
  if (playGetInFromAnimProcess) {
    _waitingForTriggerWordGetInToFinish = playGetInFromAnimProcess;
    _waitingForTriggerWordGetInToFinish_setTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}


void UserIntentComponent::PushResponseToTriggerWord(const std::string& id, const TriggerWordResponseData& newState)
{
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  auto postAudioEvent = AECH::CreatePostAudioEvent( newState.postAudioEvent,
                                                    AudioMetaData::GameObjectType::Behavior,
                                                    0 );
  PushResponseToTriggerWord(id, newState.getInTrigger, postAudioEvent, newState.streamAndLightEffect);

}

void UserIntentComponent::PushResponseToTriggerWord(const std::string& id, const AnimationTrigger& getInAnimTrigger,
                                                    const AudioEngine::Multiplexer::PostAudioEvent& postAudioEvent,
                                                    StreamAndLightEffect streamAndLightEffect)
{
  std::string animName;
  auto* data_ldr = _robot->GetContext()->GetDataLoader();
  if (data_ldr->HasAnimationForTrigger(getInAnimTrigger))
  {
    const auto groupName = data_ldr->GetAnimationForTrigger(getInAnimTrigger);
    if (!groupName.empty()) {
      animName = _robot->GetAnimationComponent().GetAnimationNameFromGroup(groupName);
      if(animName.empty()){
        LOG_WARNING("UserIntentComponent.PushResponseToTriggerWord.AnimationNotFound",
                    "No animation returned for group %s",
                    groupName.c_str());
      }
    } else {
      LOG_WARNING("UserIntentComponent.PushResponseToTriggerWord.GroupNotFound",
                  "Group not found for trigger %s",
                  AnimationTriggerToString(getInAnimTrigger));
    }
  }

  PushResponseToTriggerWord(id, animName, postAudioEvent, streamAndLightEffect);
}


void UserIntentComponent::PushResponseToTriggerWord(const std::string& id, const std::string& getInAnimationName,
                                                    const AudioEngine::Multiplexer::PostAudioEvent& postAudioEvent,
                                                    StreamAndLightEffect streamAndLightEffect)
{
  RobotInterface::SetTriggerWordResponse msg;
  msg.getInAnimationTag = _tagForTriggerWordGetInCallbacks;
  msg.postAudioEvent = postAudioEvent;
  msg.getInAnimationName = getInAnimationName;

  ApplyStreamAndLightEffect(streamAndLightEffect, msg);

  PushResponseToTriggerWordInternal(id, std::move(msg));
}


void UserIntentComponent::PopResponseToTriggerWord(const std::string& id)
{
  auto compareFunc = [&id](const TriggerWordResponseEntry& entry){
    return entry.setID == id;
  };
  auto iter = std::find_if(_responseToTriggerWordMap.begin(), _responseToTriggerWordMap.end(), compareFunc);
  if (iter == _responseToTriggerWordMap.end()) {
    LOG_WARNING("UserIntentComponent.PopResponseToTriggerWord.idNotInStack",
                "request to remove id %s, but it has not set a trigger word response",
                id.c_str());
    return;
  }

  iter = _responseToTriggerWordMap.erase(iter);
  // Check to see if the top of the stack was removed, and send a new trigger response
  if (iter == _responseToTriggerWordMap.end()) {
    if (!_responseToTriggerWordMap.empty()) {
      RobotInterface::SetTriggerWordResponse intentionalCopy = _responseToTriggerWordMap.rbegin()->response;
      _robot->SendMessage(RobotInterface::EngineToRobot(std::move(intentionalCopy)));
    } else {
      RobotInterface::SetTriggerWordResponse blankMessage;
      _robot->SendMessage(RobotInterface::EngineToRobot(std::move(blankMessage)));
    }

    LOG_DEBUG("UserIntentComponent.PopResponseToTriggerWord",
              "Removed trigger word response id '%s', have %zu left on stack",
              id.c_str(),
              _responseToTriggerWordMap.size());

    if (!_responseToTriggerWordMap.empty()) {
      LOG_DEBUG("UserIntentComponent.AfterPop",
                "After pop, top of stack has id '%s' (streaming %s, fake streaming %s, get in '%s')",
                _responseToTriggerWordMap.rbegin()->setID.c_str(),
                _responseToTriggerWordMap.rbegin()->response.shouldTriggerWordStartStream ? "enabled" : "disabled",
                _responseToTriggerWordMap.rbegin()->response.shouldTriggerWordSimulateStream ? "enabled" : "disabled",
                _responseToTriggerWordMap.rbegin()->response.getInAnimationName.c_str());
    }
  }
}


void UserIntentComponent::AlterStreamStateForCurrentResponse(const std::string& id,
                                                             const StreamAndLightEffect newEffect)
{
  if (_responseToTriggerWordMap.empty()) {
    LOG_WARNING("UserIntentComponent.AlterStreamStateForCurrentResponse.NoResponseToAlter",
                "request id '%s'",
                 id.c_str());
    return;
  }

  auto intentionalCopy = _responseToTriggerWordMap.rbegin()->response;

  ApplyStreamAndLightEffect(newEffect, intentionalCopy);

  if (_responseToTriggerWordMap.rbegin()->setID != id ||
     intentionalCopy != _responseToTriggerWordMap.rbegin()->response) {
    PushResponseToTriggerWordInternal(id, std::move(intentionalCopy));
  }

}

void UserIntentComponent::ApplyStreamAndLightEffect(const StreamAndLightEffect streamAndLightEffect,
                                                    RobotInterface::SetTriggerWordResponse& message)
{
  // anim process will open a stream if shouldStream, but will skip sending anything to the cloud if
  // simulatedStreaming. So, to get a streaming light without streaming, use StreamingDisabledButWithLight,
  // which needs to "open a stream" to get the lights to work (currently) but also sets "simulate" to true so
  // no audio data actually gets sent and we don't do a real request
  const bool simulatedStreaming = (streamAndLightEffect == StreamAndLightEffect::StreamingDisabledButWithLight);
  const bool shouldStream = (streamAndLightEffect == StreamAndLightEffect::StreamingEnabled) || simulatedStreaming;

  message.shouldTriggerWordStartStream = shouldStream;
  message.shouldTriggerWordSimulateStream = simulatedStreaming;
}

void UserIntentComponent::PushResponseToTriggerWordInternal(const std::string& id,
                                                            RobotInterface::SetTriggerWordResponse&& response)
{
  auto compareFunc = [&id](const TriggerWordResponseEntry& entry){
    return entry.setID == id;
  };
  auto iter = std::find_if(_responseToTriggerWordMap.begin(), _responseToTriggerWordMap.end(), compareFunc);

  if (iter != _responseToTriggerWordMap.end()) {
    LOG_WARNING("UserIntentComponent.PushResponseToTriggerWord.idAlreadyPushedResponse",
                "id %s already in use, removing old entry and adding new response to top of the stack",
                id.c_str());
    _responseToTriggerWordMap.erase(iter);
  }

  LOG_DEBUG("UserIntentComponent.PushResponseToTriggerWord",
            "Pushing trigger word response id '%s' (streaming %s, fake streaming %s, get in '%s')",
            id.c_str(),
            response.shouldTriggerWordStartStream ? "enabled" : "disabled",
            response.shouldTriggerWordSimulateStream ? "enabled" : "disabled",
            response.getInAnimationName.c_str());

  RobotInterface::SetTriggerWordResponse intentionalCopy = response;
  _robot->SendMessage(RobotInterface::EngineToRobot( std::move(intentionalCopy)) );
  _responseToTriggerWordMap.emplace_back(TriggerWordResponseEntry(id, std::move(response)));

}


void UserIntentComponent::DisableEngineResponseToTriggerWord( const std::string& disablerName,  bool disable )
{
  if (disable) {
    const auto res = _disableTriggerWordNames.insert(disablerName).second;
    if (!res) {
      LOG_WARNING("UserIntentComponent.DisableEngineResponseToTriggerWord.AlreadyDisabled",
                  "%s is attempting to disable the trigger word response, but it's already locking the trigger word",
                  disablerName.c_str());
    }
  } else {
    const auto numRemoved = _disableTriggerWordNames.erase(disablerName);
    if (numRemoved == 0) {
      LOG_WARNING("UserIntentComponent.DisableEngineResponseToTriggerWord.DisablerNotDisablingTrigger",
                  "%s is attempting to enable the trigger word, but it's not disabling it",
                  disablerName.c_str());
    }
  }
}

void UserIntentComponent::OnCloudData(CloudMic::Message&& data)
{
  LOG_DEBUG("UserIntentComponent.OnCloudData", "'%s'", CloudMic::MessageTagToString(data.GetTag()) );

  std::lock_guard<std::mutex> lock{_mutex};
  _pendingCloudIntent = std::move(data);
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

    if (!_whitelistedIntents.empty()) {
      // only pass on whitelisted intents
      if (_whitelistedIntents.find(userIntentTag) == _whitelistedIntents.end()) {
        LOG_INFO( "UserIntentComponent.IgnoringNonWhitelist.App", "Ignoring intent %s", UserIntentTagToString(userIntentTag) );
        const static UserIntent unmatchedIntent = UserIntent::Createunmatched_intent({});
        intent = unmatchedIntent;
      }
    }

    DevSetUserIntentPending( std::move(intent), UserIntentSource::App );
  }
}

std::string UserIntentComponent::GetServerName(const Robot& robot) const
{
  // Offset port by robotID so that we can run sims with multiple robots
  return ((robot.GetID() == 0)
           ? ""
           : std::to_string(robot.GetID()));
}

std::vector<std::string> UserIntentComponent::DevGetCloudIntentsList() const
{
  return _intentMap->DevGetCloudIntentsList();
}

std::vector<std::string> UserIntentComponent::DevGetAppIntentsList() const
{
  return _intentMap->DevGetAppIntentsList();
}

void UserIntentComponent::HandleTriggerWordEventForDas(const RobotInterface::TriggerWordDetected& msg)
{
  DASMSG(wakeword_triggered, "wakeword.triggered", "Wake word was detected");
  DASMSG_SET(i1, msg.triggerScore, "Score");
  DASMSG_SET(i2, msg.isButtonPress, "Source (0=Voice, 1=Button)");
  DASMSG_SET(i3, msg.willOpenStream, "Will stream (0=No, 1=Yes)");
  DASMSG_SEND();
}

void UserIntentComponent::SendWebVizIntents()
{
  if (_context != nullptr) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender("intents",
                                                                      _context->GetWebService()) ) {

      Json::Value& toSend = webSender->Data();
      toSend = Json::arrayValue;

      {
        Json::Value blob;
        blob["intentType"] = "user";
        blob["type"] = "current-intent";
        blob["value"] = UserIntentTagToString( _pendingIntent->intent.GetTag() );
        toSend.append(blob);
      }

      if( !_devLastReceivedCloudIntent.empty() ) {
        Json::Value blob;
        blob["intentType"] = "cloud";
        blob["type"] = "current-intent";
        blob["value"] = _devLastReceivedCloudIntent;
        toSend.append(blob);
        _devLastReceivedCloudIntent.clear();
      }

      if( !_devLastReceivedAppIntent.empty() ) {
        Json::Value blob;
        blob["intentType"] = "app";
        blob["type"] = "current-intent";
        blob["value"] = _devLastReceivedAppIntent;
        toSend.append(blob);
        _devLastReceivedAppIntent.clear();
      }
    } // if (webSender ...
  }
}

void UserIntentComponent::SetupConsoleFuncs()
{
  if (ANKI_DEV_CHEATS) {
    // allows developers to enable the trigger word in any behavior. there's no pop operation, so this is
    // just for testing (and all of this might change due to concurrent refactoring)
    auto enableTrigger = [this](ConsoleFunctionContextRef context) {
      namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
      const auto earConBegin = AudioMetaData::GameEvent::GenericEventFromString("Play__Robot_Vic_Sfx__Wake_Word_On");
      auto postAudioEvent = AECH::CreatePostAudioEvent( earConBegin, AudioMetaData::GameObjectType::Behavior, 0 );
      PushResponseToTriggerWord( "UserIntentComponent",
                                 kPlayGetInAfterDevWakeWord
                                   ? AnimationTrigger::VC_ListeningGetIn
                                   : AnimationTrigger::Count,
                                 postAudioEvent, // required if there is to be any effect at all
                                 kStreamAfterDevWakeWord
                                   ? StreamAndLightEffect::StreamingEnabled
                                   : StreamAndLightEffect::StreamingDisabled
                                );
    };
    _consoleFuncs.emplace_front( "EnableDevTriggerWord", std::move(enableTrigger), "UserIntentComponent", "" );
  }
}

}
}
