/**
 * File: ICozmoBehavior.cpp
 *
 * Author: Andrew Stein : Kevin M. Karol
 * Date:   7/30/15  : 12/1/16
 *
 * Description: Defines interface for a Cozmo "Behavior"
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/actions/actionInterface.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTriggerWordPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "engine/aiComponent/continuityComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/powerStateManager.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/unitTestKey.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/activeFeatures.h"
#include "clad/types/behaviorComponent/behaviorClasses.h"
#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/behaviorComponent/behaviorTriggerResponse.h"
#include "clad/types/behaviorComponent/streamAndLightEffect.h"
#include "clad/types/behaviorComponent/userIntent.h"

#include "proto/external_interface/shared.pb.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"
#include "util/fileUtils/fileUtils.h"
#include "util/math/numericCast.h"

#include "webServerProcess/src/webVizSender.h"

#define LOG_CHANNEL    "Behaviors"

namespace Anki {
namespace Vector {

namespace {
static const char* kBehaviorClassKey                 = "behaviorClass";
static const char* kBehaviorIDConfigKey              = "behaviorID";

static const char* kRequiredDriveOffChargerKey       = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey          = "requiredRecentSwitchToParent_sec";
static const char* kExecutableBehaviorTypeKey        = "executableBehaviorType";
static const char* kAlwaysStreamlineKey              = "alwaysStreamline";
static const char* kWantsToBeActivatedCondConfigKey  = "wantsToBeActivatedCondition";
static const char* kWantsToCancelSelfConfigKey       = "wantsToCancelSelfCondition";
static const char* kRespondToUserIntentsKey          = "respondToUserIntents";
static const char* kRespondToTriggerWordKey          = "respondToTriggerWord";
static const char* kResetTimersKey                   = "resetTimers";
static const char* kEmotionEventOnActivatedKey       = "emotionEventOnActivated";
static const char* kPostBehaviorSuggestionKey        = "postBehaviorSuggestion";
static const char* kAssociatedActiveFeature          = "associatedActiveFeature";
static const char* kBehaviorStatToIncrement          = "behaviorStatToIncrement";
static const char* kTracksToLockWhileActivatedKey    = "tracksToLockWhileActivated";
static const char* kAlterStreamAfterWakewordStateKey = "alterStreamAfterWakeword";
static const char* kPushTriggerWordResponseKey       = "pushTriggerWordResponse";

static const std::string kIdleLockPrefix             = "Behavior_";

// Keys for loading in anonymous behaviors
static const char* kAnonymousBehaviorMapKey          = "anonymousBehaviors";
static const char* kAnonymousBehaviorName            = "behaviorName";

static const char* kBehaviorDebugLabel               = "debugLabel";
static const char* kTracksLockedWhileActivatedID     = "tracksLockedWhileActivated";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass behaviorClass, BehaviorID behaviorID)
{
  Json::Value config;
  config[kBehaviorClassKey] = BehaviorTypesWrapper::BehaviorClassToString(behaviorClass);
  config[kBehaviorIDConfigKey] = BehaviorTypesWrapper::BehaviorIDToString(behaviorID);
  return config;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::InjectBehaviorClassAndIDIntoConfig(BehaviorClass behaviorClass, BehaviorID behaviorID, Json::Value& config)
{
  if(config.isMember(kBehaviorIDConfigKey)){
    PRINT_NAMED_WARNING("ICozmoBehavior.InjectBehaviorIDIntoConfig.BehaviorIDAlreadyExists",
                        "Overwriting behaviorID %s with behaviorID %s",
                        config[kBehaviorIDConfigKey].asString().c_str(),
                        BehaviorTypesWrapper::BehaviorIDToString(behaviorID));
  }
  if(config.isMember(kBehaviorClassKey)){
    PRINT_NAMED_WARNING("ICozmoBehavior.InjectBehaviorIDIntoConfig.BehaviorClassAlreadyExists",
                        "Overwriting behaviorClass %s with behaviorClass %s",
                        config[kBehaviorClassKey].asString().c_str(),
                        BehaviorTypesWrapper::BehaviorClassToString(behaviorClass));
  }

  config[kBehaviorIDConfigKey] = BehaviorTypesWrapper::BehaviorIDToString(behaviorID);
  config[kBehaviorClassKey] = BehaviorTypesWrapper::BehaviorClassToString(behaviorClass);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorID ICozmoBehavior::ExtractBehaviorIDFromConfig(const Json::Value& config,
                                                       const std::string& fileName)
{
  const std::string debugName = "IBeh.NoBehaviorIdSpecified";
  const std::string behaviorID_str = JsonTools::ParseString(config, kBehaviorIDConfigKey, debugName);

  // To make it easy to find behaviors, assert that the file name and behaviorID match
  if(ANKI_DEV_CHEATS && !fileName.empty()){
    std::string jsonFileName = Util::FileUtils::GetFileName(fileName);
    auto dotIndex = jsonFileName.find_last_of(".");
    std::string lowerFileName = dotIndex == std::string::npos ? jsonFileName : jsonFileName.substr(0, dotIndex);
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

    std::string behaviorIDLower = behaviorID_str;
    std::transform(behaviorIDLower.begin(), behaviorIDLower.end(), behaviorIDLower.begin(), ::tolower);
    DEV_ASSERT_MSG(behaviorIDLower == lowerFileName,
                   "RobotDataLoader.LoadBehaviors.BehaviorIDFileNameMismatch",
                   "File name %s does not match BehaviorID %s",
                   fileName.c_str(),
                   behaviorID_str.c_str());
  }

  return BehaviorTypesWrapper::BehaviorIDFromString(behaviorID_str);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass ICozmoBehavior::ExtractBehaviorClassFromConfig(const Json::Value& config)
{
  const Json::Value& behaviorTypeJson = config[kBehaviorClassKey];
  const char* behaviorTypeString = behaviorTypeJson.isString() ? behaviorTypeJson.asCString() : "";
  return BehaviorTypesWrapper::BehaviorClassFromString(behaviorTypeString);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ICozmoBehavior::MakeUniqueDebugLabelFromConfig(const Json::Value& config)
{
  std::string ret;
  const BehaviorID behaviorID = ExtractBehaviorIDFromConfig( config );
  if( config.isMember(kBehaviorDebugLabel) ) {
    ret = config[kBehaviorDebugLabel].asString();
  } else {
    ret = BehaviorTypesWrapper::BehaviorIDToString( behaviorID );
  }
  // now make it unique and append the instance # if not the very first
  // this will be unique per BehaviorID instead of per "kBehaviorDebugLabel," but they're usually the same
  static std::unordered_map<BehaviorID, unsigned int> counts;
  const auto index = counts[behaviorID]++; // and post-increment
  if( index > 0 ) {
    ret += std::to_string(index);
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::ICozmoBehavior(const Json::Value& config)
  : ICozmoBehavior(config, {})
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::ICozmoBehavior(const Json::Value& config, const CustomBEIConditionHandleList& customConditionHandles)
: IBehavior( MakeUniqueDebugLabelFromConfig( config ) )
, _lastRunTime_s(0.0f)
, _activatedTime_s(0.0f)
, _id(ExtractBehaviorIDFromConfig(config))
, _behaviorClassID(ExtractBehaviorClassFromConfig(config))
, _executableType(BehaviorTypesWrapper::GetDefaultExecutableBehaviorType())
, _intentToDeactivate( UserIntentTag::INVALID )
, _respondToTriggerWord( false )
, _emotionEventOnActivated("")
, _requiredRecentDriveOffCharger_sec(-1.0f)
, _requiredRecentSwitchToParent_sec(-1.0f)
, _isActivated(false)
{
  if(!ReadFromJson(config)){
    PRINT_NAMED_WARNING("ICozmoBehavior.ReadFromJson.Failed",
                        "Something went wrong reading from JSON");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::ReadFromJson(const Json::Value& config)
{
  // - - - - - - - - - -
  // Got off charger timer
  const Json::Value& requiredDriveOffChargerJson = config[kRequiredDriveOffChargerKey];
  if (!requiredDriveOffChargerJson.isNull())
  {
    DEV_ASSERT_MSG(requiredDriveOffChargerJson.isNumeric(), "ICozmoBehavior.ReadFromJson", "Not a float: %s",
                   kRequiredDriveOffChargerKey);
    _requiredRecentDriveOffCharger_sec = requiredDriveOffChargerJson.asFloat();
  }

  // - - - - - - - - - -
  // Required recent parent switch
  const Json::Value& requiredSwitchToParentJson = config[kRequiredParentSwitchKey];
  if (!requiredSwitchToParentJson.isNull()) {
    DEV_ASSERT_MSG(requiredSwitchToParentJson.isNumeric(), "ICozmoBehavior.ReadFromJson", "Not a float: %s",
                   kRequiredParentSwitchKey);
    _requiredRecentSwitchToParent_sec = requiredSwitchToParentJson.asFloat();
  }

  const Json::Value& executableBehaviorTypeJson = config[kExecutableBehaviorTypeKey];
  if (executableBehaviorTypeJson.isString())
  {
    _executableType = BehaviorTypesWrapper::ExecutableBehaviorTypeFromString(executableBehaviorTypeJson.asCString());
  }

  JsonTools::GetValueOptional(config, kAlwaysStreamlineKey, _alwaysStreamline);

  // Add WantsToBeActivated conditions
  if(config.isMember(kWantsToBeActivatedCondConfigKey)){
    const auto& condition = config[kWantsToBeActivatedCondConfigKey];
    _wantsToBeActivatedConditions.push_back(
      BEIConditionFactory::CreateBEICondition(condition, GetDebugLabel() )
    );
  }

  // Add WantsToCancelSelf conditions
  if(config.isMember(kWantsToCancelSelfConfigKey)){
    const auto& condition = config[kWantsToCancelSelfConfigKey];
    _wantsToCancelSelfConditions.push_back(
      BEIConditionFactory::CreateBEICondition( condition, GetDebugLabel() )
    );
  }

  if(config.isMember(kAnonymousBehaviorMapKey)){
    _anonymousBehaviorMapConfig = config[kAnonymousBehaviorMapKey];
  }

  if(config.isMember(kRespondToUserIntentsKey)){
    // create a ConditionUserIntentPending based on the config json
    Json::Value json = ConditionUserIntentPending::GenerateConfig( config[kRespondToUserIntentsKey] );
    auto cond = BEIConditionFactory::CreateBEICondition( json, GetDebugLabel() );
    auto castCond = std::dynamic_pointer_cast<ConditionUserIntentPending>(cond);
    if( ANKI_VERIFY( castCond != nullptr,
                     "ICozmoBehavior.ReadFromJson.InvalidCondPtr",
                     "Could not cast a factory-made condition to the subclass type") )
    {
      _respondToUserIntent = castCond;
      _wantsToBeActivatedConditions.push_back( cond );
    }
  }

  _respondToTriggerWord = config.get(kRespondToTriggerWordKey, false).asBool();

  _emotionEventOnActivated = config.get(kEmotionEventOnActivatedKey, "").asString();

  JsonTools::GetVectorOptional(config, kResetTimersKey, _resetTimers);
  for( const auto& timerName : _resetTimers ) {
    ANKI_VERIFY( BehaviorTimerManager::IsValidName( timerName ),
                 "ICozmoBehavior.ReadFromJson.InvalidTimer",
                 "Timer '%s' is invalid",
                 timerName.c_str() );
  }

  if( config[kPostBehaviorSuggestionKey].isString() ) {
    const auto& suggestionStr = config[kPostBehaviorSuggestionKey].asString();
    ANKI_VERIFY( PostBehaviorSuggestionsFromString( suggestionStr, _postBehaviorSuggestion ),
                 "ICozmoBehavior.ReadFromJson.InvalidPostBehSug",
                 "Suggestion '%s' invalid in behavior '%s'",
                 suggestionStr.c_str(),
                 GetDebugLabel().c_str() );
  }

  if( config[kAssociatedActiveFeature].isString() ) {
    _associatedActiveFeature.reset(new ActiveFeature);
    const auto& featureStr = config[kAssociatedActiveFeature].asString();
    ANKI_VERIFY( ActiveFeatureFromString( featureStr, *_associatedActiveFeature ),
                 "ICozmoBehavior.ReadFromJson.InvalidActiveFeature",
                 "Active feature '%s' invalid in behavior '%s'",
                 featureStr.c_str(),
                 GetDebugLabel().c_str() );
  }

  if( config[kBehaviorStatToIncrement].isString() ) {
    _behaviorStatToIncrement.reset(new BehaviorStat);
    const auto& statStr = config[kBehaviorStatToIncrement].asString();
    ANKI_VERIFY( BehaviorStatFromString( statStr, *_behaviorStatToIncrement ),
                 "ICozmoBehavior.ReadFromJson.InvalidBehaviorState",
                 "Behavior stat to increment '%s' invalid in behavior '%s'",
                 statStr.c_str(),
                 GetDebugLabel().c_str() );
  }
  
  // tracks to lock while activated
  if(config[kTracksToLockWhileActivatedKey].isArray()){
    for(const auto& track : config[kTracksToLockWhileActivatedKey]){
      _tracksToLockWhileActivated |= static_cast<u8>(AnimTrackFlagFromString(track.asString()));
    }
  }            
  
  {
    StreamAndLightEffect alterState;
    const bool assertIfMissing = false; // key is optional
    if( JsonTools::GetCladEnumFromJSON(config,
                                       kAlterStreamAfterWakewordStateKey,
                                       alterState,
                                       GetDebugLabel(),
                                       assertIfMissing) ) {
      _alterStreamAfterWakeword = std::make_unique<StreamAndLightEffect>(std::move(alterState));
    }
  }

  {
    TriggerWordResponseData triggerStateToPush;
    if( config[kPushTriggerWordResponseKey].isObject() &&
      triggerStateToPush.SetFromJSON( config[ kPushTriggerWordResponseKey ] ) ) {

      if( ANKI_VERIFY(_alterStreamAfterWakeword == nullptr,
                      "ICozmoBehavior.ReadFromJson.MultipleStreamStateArguments",
                      "Behavior '%s' specified both '%s' and '%s', pick one!",
                      GetDebugLabel().c_str(),
                      kAlterStreamAfterWakewordStateKey,
                      kPushTriggerWordResponseKey) ) {
        _triggerStreamStateToPush = std::make_unique<TriggerWordResponseData>(std::move(triggerStateToPush));
      }
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::CheckJson(const Json::Value& config)
{
  const std::vector<const char*> expectedKeys = GetAllJsonKeys();
  std::vector<std::string> badKeys;
  const bool hasBadKeys = JsonTools::HasUnexpectedKeys( config, expectedKeys, badKeys );
  if( hasBadKeys ) {
    std::string keys;
    for( const auto& key : badKeys ) {
      keys += key;
      keys += ",";
    }
    ANKI_VERIFY( false,
                 "BehaviorContainer.CreateAndStoreBehavior.UnexpectedKey",
                 "Behavior '%s' has unexpected keys '%s'",
                 GetDebugLabel().c_str(),
                 keys.c_str() );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::~ICozmoBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<const char*> ICozmoBehavior::GetAllJsonKeys() const
{
  std::vector<const char*> expectedKeys;

  // this comes first so that if the behavior assigns instead of inserts its more likely to fail.
  static const char* baseKeys[] = {
    kBehaviorClassKey,
    kBehaviorIDConfigKey,
    kRequiredDriveOffChargerKey,
    kRequiredParentSwitchKey,
    kExecutableBehaviorTypeKey,
    kAlwaysStreamlineKey,
    kWantsToBeActivatedCondConfigKey,
    kWantsToCancelSelfConfigKey,
    kRespondToUserIntentsKey,
    kRespondToTriggerWordKey,
    kEmotionEventOnActivatedKey,
    kResetTimersKey,
    kAnonymousBehaviorMapKey,
    kPostBehaviorSuggestionKey,
    kAssociatedActiveFeature,
    kBehaviorStatToIncrement,
    kTracksToLockWhileActivatedKey,
    kAlterStreamAfterWakewordStateKey,
    kPushTriggerWordResponseKey
  };
  expectedKeys.insert( expectedKeys.end(), std::begin(baseKeys), std::end(baseKeys) );

  if( _id == BEHAVIOR_ID(Anonymous) ) {
    // keys only for anonymous behavior
    static const char* anonKeys[] = {
      kAnonymousBehaviorName,
      kBehaviorDebugLabel,
    };
    expectedKeys.insert( expectedKeys.end(), std::begin(anonKeys), std::end(anonKeys) );
  }

  std::set<const char*> behaviorKeys;
  GetBehaviorJsonKeys( behaviorKeys );
  expectedKeys.insert( expectedKeys.end(), behaviorKeys.begin(), behaviorKeys.end() );

  return expectedKeys;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::InitInternal()
{
  _initHasBeenCalled = true;

  {
    for( auto& condition : _wantsToBeActivatedConditions ) {
      condition->Init(GetBEI());
    }

    for( auto& condition : _wantsToCancelSelfConditions ) {
      condition->Init(GetBEI());
    }

    if( _respondToTriggerWord ){
      IBEIConditionPtr condition(BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel()));
      condition->Init(GetBEI());
      _wantsToBeActivatedConditions.push_back(condition);
    }

  }

  if(!_anonymousBehaviorMapConfig.empty()){
    for(auto& entry: _anonymousBehaviorMapConfig){
      const std::string debugStr = "ICozmoBehavior.ReadFromJson.";

      const std::string behaviorName = JsonTools::ParseString(entry, kAnonymousBehaviorName, debugStr + "BehaviorNameMissing");

      const bool isBehaviorID = BehaviorTypesWrapper::IsValidBehaviorID(behaviorName);
      ANKI_VERIFY(!isBehaviorID,
                  "ICozmoBehavior.InitInternal.AnonymousNameIsABehaviorID",
                  "behavior '%s' declares an anonymous behavior named '%s', but that matches an existing behavior ID",
                  GetDebugLabel().c_str(),
                  behaviorName.c_str());

      if( !entry.isMember( kBehaviorDebugLabel ) ) {
        entry[kBehaviorDebugLabel] = "@" + behaviorName;
      } else {
        PRINT_NAMED_ERROR("ICozmoBehavior.InitInternal.AnonymousParamIsReserved",
                          "anon behavior '%s' contains the reserved field '%s'",
                          GetDebugLabel().c_str(),
                          kBehaviorDebugLabel );
      }

      if( entry.isMember( kBehaviorIDConfigKey ) ) {
        PRINT_NAMED_ERROR("ICozmoBehavior.InitInternal.AnonymousBehaviorHasID",
                          "Anonymous behaviors cannot contain IDs. Anonymous behavior '%s' defined in behavior '%s'"
                          "had id '%s'. Overwriting with 'Anonymous'",
                          behaviorName.c_str(),
                          GetDebugLabel().c_str(),
                          entry[kBehaviorIDConfigKey].asCString());
      }

      entry[kBehaviorIDConfigKey] = BehaviorTypesWrapper::BehaviorIDToString(BEHAVIOR_ID(Anonymous));

      // check for duplicate behavior names since maps require a unique key
      auto it = _anonymousBehaviorMap.find(behaviorName);
      if(it == _anonymousBehaviorMap.end()){
        ICozmoBehaviorPtr newBehavior = BehaviorFactory::CreateBehavior(entry);
        if( ANKI_VERIFY(newBehavior != nullptr,
                        "ICozmoBehavior.InitInternal.FailedToCreateAnonymousBehavior",
                        "Could not create anonymous behavior with name '%s'",
                        behaviorName.c_str()) ) {

          auto resultPair = _anonymousBehaviorMap.insert(std::make_pair(behaviorName, newBehavior));

          DEV_ASSERT_MSG(resultPair.first->second != nullptr,
                         "ICozmoBehavior.InitInternal.FailedToAllocateAnonymousBehavior",
                         "Failed to allocate new anonymous behavior (%s) within behavior %s",
                         behaviorName.c_str(),
                         GetDebugLabel().c_str());

          // we need to initialize the anon behaviors as well
          newBehavior->Init(GetBEI());
          newBehavior->InitBehaviorOperationModifiers();
        }
      }
      else
      {
        PRINT_NAMED_ERROR("ICozmoBehavior.InitInternal.DuplicateAnonymousBehaviorName",
                          "Duplicate anonymous behavior name (%s) found for behavior '%s'",
                          behaviorName.c_str(),
                          GetDebugLabel().c_str());
      }
    }
  }
  // Don't need the map anymore, plus map is altered as part of iteration so it's no longer valid
  _anonymousBehaviorMapConfig.clear();

  if( ! _emotionEventOnActivated.empty() ) {
    ANKI_VERIFY( GetBEI().GetMoodManager().IsValidEmotionEvent(_emotionEventOnActivated),
                 "ICozmoBehavior.Init.InvalidEmotionEvent",
                 "Behavior '%s' specifies emotion event '%s' which has not been loaded by the mood manager",
                 GetDebugLabel().c_str(),
                 _emotionEventOnActivated.c_str());
  }

  // Allow internal init to happen before subscribing to tags in case additional
  // tags are added
  InitBehavior();

  ///////
  //// Subscribe to tags
  ///////

  for(auto tag : _gameToEngineTags) {
    GetBEI().GetBehaviorEventComponent().SubscribeToTags(this, {tag});
  }

  for(auto tag : _engineToGameTags) {
    GetBEI().GetBehaviorEventComponent().SubscribeToTags(this, {tag});
  }

  for(auto tag: _robotToEngineTags) {
    GetBEI().GetBehaviorEventComponent().SubscribeToTags(this,{tag});
  }
  
  for(auto tag : _appToEngineTags) {
    GetBEI().GetBehaviorEventComponent().SubscribeToTags(this, {tag});
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::InitBehaviorOperationModifiers()
{
  // N.B. this can't happen in Init because some behaviors actually rely on other behaviors having been
  // initialized to properly handler GetBehaviorOperationModifiers

  GetBehaviorOperationModifiers(_operationModifiers);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SetDontActivateThisTick(const std::string& coordinatorName)
{
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();

  if( currTick - _tickDontActivateSetFor > 2 ) {
    // only print if we were "recently" stopped from being activated
    PRINT_CH_DEBUG("Behaviors", "ICozmoBehavior.SetDontActivateThisTick",
                   "Behavior '%s' being suppresses by coordinator '%s'",
                   GetDebugLabel().c_str(),
                   coordinatorName.c_str());
  }

  _dontActivateCoordinator = coordinatorName;
  _tickDontActivateSetFor = BaseStationTimer::getInstance()->GetTickCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::GetAssociatedActiveFeature(ActiveFeature& feature) const
{
  if( _associatedActiveFeature != nullptr ) {
    feature = *_associatedActiveFeature;
    return true;
  }
  else {
    return false;
  }
}      

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string,ICozmoBehaviorPtr> ICozmoBehavior::TESTONLY_GetAnonBehaviors( UnitTestKey key ) const
{
  return _anonymousBehaviorMap;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::AddWaitForUserIntent( UserIntentTag intentTag )
{
  if( !ANKI_VERIFY( !_initHasBeenCalled,
                    "ICozmoBehavior.AddWaitForUserIntent.TagAfterInit",
                    "behavior '%s' trying to set user intent '%s' after init has already been called",
                    GetDebugLabel().c_str(),
                    UserIntentTagToString(intentTag) ) )
  {
    return;
  }
  if( !ANKI_VERIFY( intentTag != USER_INTENT(INVALID),
                    "ICozmoBehavior.AddWaitForUserIntent.TagInvalid",
                    "behavior '%s' trying to set invalid user intent. use ClearWaitForUserIntent",
                    GetDebugLabel().c_str()) )
  {
    return;
  }


  if( _respondToUserIntent == nullptr ) {
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>( GetDebugLabel() );
    _wantsToBeActivatedConditions.push_back( _respondToUserIntent );
  }
  _respondToUserIntent->AddUserIntent( intentTag );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::AddWaitForUserIntent( UserIntent&& intent )
{
  if( !ANKI_VERIFY( !_initHasBeenCalled,
                    "ICozmoBehavior.AddWaitForUserIntent.IntentAfterInit",
                    "behavior '%s' trying to set user intent '%s' after init has already been called",
                    GetDebugLabel().c_str(),
                    UserIntentTagToString(intent.GetTag() ) ) )
  {
    return;
  }
  if( !ANKI_VERIFY( intent.GetTag() != USER_INTENT(INVALID),
                    "ICozmoBehavior.AddWaitForUserIntent.IntentInvalid",
                    "behavior '%s' trying to set invalid user intent. use ClearWaitForUserIntent",
                    GetDebugLabel().c_str()) )
  {
    return;
  }


  if( _respondToUserIntent == nullptr ) {
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>( GetDebugLabel() );
    _wantsToBeActivatedConditions.push_back( _respondToUserIntent );
  }
  _respondToUserIntent->AddUserIntent( std::move(intent) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::AddWaitForUserIntent( UserIntentTag tag, EvalUserIntentFunc&& func )
{
  if( !ANKI_VERIFY( !_initHasBeenCalled,
                    "ICozmoBehavior.AddWaitForUserIntent.FuncAfterInit",
                    "behavior '%s' trying to set user intent '%s' after init has already been called",
                    GetDebugLabel().c_str(),
                    UserIntentTagToString(tag) ) )
  {
    return;
  }
  if( !ANKI_VERIFY( tag != USER_INTENT(INVALID),
                    "ICozmoBehavior.AddWaitForUserIntent.FuncTagInvalid",
                    "behavior '%s' trying to set invalid user intent. use ClearWaitForUserIntent",
                    GetDebugLabel().c_str()) )
  {
    return;
  }

  if( !ANKI_VERIFY( func != nullptr,
                   "ICozmoBehavior.AddWaitForUserIntent.FuncInvalid",
                   "behavior '%s' trying to set invalid lambda. use an overloaded method",
                   GetDebugLabel().c_str()) )
  {
    return;
  }


  if( _respondToUserIntent == nullptr ) {
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>( GetDebugLabel() );
    _wantsToBeActivatedConditions.push_back( _respondToUserIntent );
  }
  _respondToUserIntent->AddUserIntent( tag, std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::ClearWaitForUserIntent()
{
  if( _respondToUserIntent != nullptr ) {
    auto remIt = std::remove_if( _wantsToBeActivatedConditions.begin(),
                                 _wantsToBeActivatedConditions.end(),
                                 [&ref=_respondToUserIntent](const IBEIConditionPtr ptr) {
                                   return ref == ptr;
                                 });
    _wantsToBeActivatedConditions.erase( remIt );
    _respondToUserIntent.reset();
  } else {
    PRINT_NAMED_WARNING( "ICozmoBehavior.ClearWaitForUserIntent.Empty",
                         "behavior '%s' clearing wait for user intent, but it was never set",
                         GetDebugLabel().c_str() );

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SetRespondToTriggerWord(bool shouldRespond)
{
  if( ANKI_VERIFY( !_initHasBeenCalled,
                   "ICozmoBehavior.SetRespondToTriggerWord.AfterInit",
                   "behavior '%s' trying to set trigger word after init has already been called",
                   GetDebugLabel().c_str()) ) {
    if( _respondToTriggerWord ) {
      PRINT_NAMED_WARNING("ICozmoBehavior.SetRespondToTriggerWord.Replace",
                          "behavior '%s' setting should respond to %d, but it was previously %d",
                          GetDebugLabel().c_str(),
                          shouldRespond,
                          _respondToTriggerWord);
    }

    _respondToTriggerWord = shouldRespond;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  _gameToEngineTags.insert(tags.begin(), tags.end());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  _engineToGameTags.insert(tags.begin(), tags.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<RobotInterface::RobotToEngineTag> &&tags)
{
  _robotToEngineTags.insert(tags.begin(), tags.end());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToAppTags(std::set<AppToEngineTag>&& tags)
{
  _appToEngineTags.insert(tags.begin(), tags.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnActivatedInternal()
{
  PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".Init").c_str(), "Starting...");

  // Check if there are any engine-generated actions in the action list, because there shouldn't be!
  // If there is, a behavior probably didn't use DelegateIfInControl() where it should have.
  /**bool engineActionStillRunning = false;
  for (auto listIt = robot.GetActionList().begin();
       listIt != robot.GetActionList().end() && !engineActionStillRunning;
       ++listIt) {

    const ActionQueue& q = listIt->second;

    // Start with current action list
    const IActionRunner* currRunningAction = q.GetCurrentRunningAction();
    if ((nullptr != currRunningAction) &&
        (currRunningAction->GetTag() >= ActionConstants::FIRST_ENGINE_TAG) &&
        (currRunningAction->GetTag() <= ActionConstants::LAST_ENGINE_TAG)) {
      engineActionStillRunning = true;
    }

    // Check actions in queue
    for (auto it = q.begin(); it != q.end() && !engineActionStillRunning; ++it) {
      u32 tag = (*it)->GetTag();
      if ((tag >= ActionConstants::FIRST_ENGINE_TAG) && (tag <= ActionConstants::LAST_ENGINE_TAG)) {
        engineActionStillRunning = true;
      }
    }
  }

  if (engineActionStillRunning) {
    PRINT_NAMED_WARNING("ICozmoBehavior.Init.ActionsInQueue",
                        "Initializing %s: %zu actions already in queue",
                        GetDebugLabel().c_str(), robot.GetActionList().GetQueueLength(0));
  }**/

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _isActivated = true;
  _delegationCallback = nullptr;
  _activatedTime_s = currTime_s;
  _startCount++;

  // Activate user intent if responding to it. Since _respondToUserIntent is initialized, this
  // behavior has a BEI condition that won't be true unless the specific user intent is pending.
  // If this behavior was activated, then it must have been true, so it suffices to check if it's
  // initialized instead of evaluating it here.
  if( _respondToUserIntent != nullptr ) {
    auto tag = _respondToUserIntent->GetUserIntentTagSelected();
    if( tag != USER_INTENT(INVALID) ) {
      SmartActivateUserIntent( tag );
    } else {
      // this can happen in tests
      PRINT_NAMED_WARNING("ICozmoBehavior.OnActivatedInternal.InvalidTag",
                          "_respondToUserIntent said it was responding to the INVALID tag" );
    }
  }
  if(_tracksToLockWhileActivated != 0){
    SmartLockTracks(_tracksToLockWhileActivated, kTracksLockedWhileActivatedID, GetDebugLabel());
  }

  // Clear trigger word if responding to it
  if( _respondToTriggerWord ) {
    GetBehaviorComp<UserIntentComponent>().ClearPendingTriggerWord();
  }
  
  if( _alterStreamAfterWakeword != nullptr ) {
    SmartAlterStreamStateForCurrentResponse(*_alterStreamAfterWakeword);
  }

  if( _triggerStreamStateToPush != nullptr ) {
    SmartPushResponseToTriggerWord(*_triggerStreamStateToPush);
  }

  // Handle Vision Mode Subscriptions
  if(!_operationModifiers.visionModesForActiveScope->empty()){
    GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this,
      *_operationModifiers.visionModesForActiveScope);
  }

  // Manage state for any WantsToBeActivated conditions used by this Behavior
  // Conditions may not be evaluted when the behavior is Active
  for(auto& condition: _wantsToBeActivatedConditions){
    condition->SetActive(GetBEI(), false);
  }

  // Manage state for any WantsToCancelSelf conditions used by this Behavior
  // Conditions will be evaluated while the behavior is activated
  for(auto& condition: _wantsToCancelSelfConditions){
    condition->SetActive(GetBEI(), true);
  }

  // reset any timers
  for( const auto& timerName : _resetTimers ) {
    auto timer = BehaviorTimerManager::BehaviorTimerFromString( timerName );
    GetBEI().GetBehaviorTimerManager().GetTimer( timer  ).Reset();
  }

  // trigger emotion event, if present
  // TODO:(bn) convert these to CLAD?
  if( !_emotionEventOnActivated.empty() ) {
    GetBEI().GetMoodManager().TriggerEmotionEvent(_emotionEventOnActivated, currTime_s);
  }

  if( _behaviorStatToIncrement ) {
    GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(*_behaviorStatToIncrement);
  }

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::BehaviorActivated);

  {
    using requirements = BehaviorOperationModifiers::CubeConnectionRequirements;
    if(requirements::None != _operationModifiers.cubeConnectionRequirements){
      ManageCubeConnectionSubscriptions(true);
    }
  }
  
  OnBehaviorActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnEnteredActivatableScopeInternal()
{
  // Handle Vision Mode Subscriptions
  if(!_operationModifiers.visionModesForActivatableScope->empty()){
    GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this,
      *_operationModifiers.visionModesForActivatableScope);
  }

  // Manage state for any IBEIConditions used by this Behavior
  // Conditions may be evaluted when the behavior is inside the Activatable Scope
  for(auto& condition: _wantsToBeActivatedConditions){
    condition->SetActive(GetBEI(), true);
  }

  OnBehaviorEnteredActivatableScope();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnLeftActivatableScopeInternal()
{
  const bool hasActivatableScopeVisionModes = !_operationModifiers.visionModesForActivatableScope->empty();
  if (hasActivatableScopeVisionModes) {
    GetBEI().GetVisionScheduleMediator().ReleaseAllVisionModeSubscriptions(this);
  }

  // Manage state for any IBEIConditions used by this Behavior
  // Conditions may not be evaluted when the behavior is outside the Activatable Scope
  for(auto& condition: _wantsToBeActivatedConditions){
    condition->SetActive(GetBEI(), false);
  }

  OnBehaviorLeftActivatableScope();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnDeactivatedInternal()
{
  PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".Stop").c_str(), "Stopping...");

  _isActivated = false;
  OnBehaviorDeactivated();
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  CancelDelegates(false);

  // Clear callbacks
  _delegationCallback = nullptr;

  const bool hasActiveScopeVisionModes = !_operationModifiers.visionModesForActiveScope->empty();
  if (hasActiveScopeVisionModes) {
    // If there are any required visions modes for ActivatableScope, reset the mode subscriptions
    // back to ActivatableScope values. OnLeftActivatableScopeInternal handles final unsubscribe.
    // If there are no activatable scope requests, then we can unsubscribe from all vision modes now.
    const bool hasActivatableScopeVisionModes = !_operationModifiers.visionModesForActivatableScope->empty();
    if (hasActivatableScopeVisionModes) {
      GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this,
        *_operationModifiers.visionModesForActivatableScope);
    } else {
      GetBEI().GetVisionScheduleMediator().ReleaseAllVisionModeSubscriptions(this);
    }
  }

  // Manage state for any WantsToBeActivated conditions used by this Behavior:
  // Conditions may be evaluted when inactive, if in Activatable scope
  for(auto& condition: _wantsToBeActivatedConditions){
    condition->SetActive(GetBEI(), true);
  }

  // Manage state for any WantsToCancelSelf conditions used by this Behavior
  // Conditions will be evaluated while the behavior is activated
  for(auto& condition: _wantsToCancelSelfConditions){
    condition->SetActive(GetBEI(), false);
  }

  // clear the path component motion profile if it was set by the behavior
  if( _hasSetMotionProfile ) {
    SmartClearMotionProfile();
  }

  // Unlock any tracks which the behavior hasn't had a chance to unlock
  for(const auto& entry: _lockingNameToTracksMap){
    GetBEI().GetRobotInfo().GetMoveComponent().UnlockTracks(entry.second, entry.first);
  }

  _lockingNameToTracksMap.clear();
  _customLightObjects.clear();
  
  if( _postBehaviorSuggestion != PostBehaviorSuggestions::Invalid ) {
    GetAIComp<AIWhiteboard>().OfferPostBehaviorSuggestion( _postBehaviorSuggestion );
  }

  if( _intentToDeactivate != UserIntentTag::INVALID ) {
    SmartDeactivateUserIntent();
  }
  
  if (_pushedCustomTriggerResponse) {
    SmartPopResponseToTriggerWord();
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsTriggerWordDisabledByName(GetDebugLabel())){
    SmartEnableEngineResponseToTriggerWord();
  }

  if( !_powerSaveRequest.empty() ) {
    GetBehaviorComp<PowerStateManager>().RemovePowerSaveModeRequest(_powerSaveRequest);
    _powerSaveRequest.clear();
  }

  {
    using requirements = BehaviorOperationModifiers::CubeConnectionRequirements;
    if(requirements::None != _operationModifiers.cubeConnectionRequirements){
      ManageCubeConnectionSubscriptions(false);
    }
  }

  if( _keepAliveDisabled ) {
    GetBEI().GetAnimationComponent().RemoveKeepFaceAliveDisableLock(GetDebugLabel());
    _keepAliveDisabled = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::WantsToBeActivatedInternal() const
{
  if(WantsToBeActivatedBase()){
    return WantsToBeActivatedBehavior();
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::WantsToBeActivatedBase() const
{
  // Factory test does not need to check behavior runnability
  if(FACTORY_TEST){
    return true;
  }

  // Some reaction trigger strategies allow behaviors to interrupt themselves.
  DEV_ASSERT(!IsActivated(), "ICozmoBehavior.WantsToBeActivatedCalledOnRunningBehavior");
  if (IsActivated()) {
    PRINT_CH_DEBUG("Behaviors", "ICozmoBehavior.WantsToBeActivatedBase", "Behavior %s is already running", GetDebugLabel().c_str());
    return true;
  }

  if(_tickDontActivateSetFor == BaseStationTimer::getInstance()->GetTickCount()){
    PRINT_PERIODIC_CH_INFO(200,"Behaviors", "ICozmoBehavior.WantsToBeActivatedBase.DontActivateDueToCoordinator",
                           "Behavior %s was [still] asked not to activate during tick %zu by coordinator %s",
                           GetDebugLabel().c_str(), _tickDontActivateSetFor, _dontActivateCoordinator.c_str());
    return false;
  }

  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // if there's a timer requiring a recent drive off the charger, check with whiteboard
  const bool requiresRecentDriveOff = FLT_GE(_requiredRecentDriveOffCharger_sec, 0.0f);
  if ( requiresRecentDriveOff )
  {
    const float lastDriveOff = GetAIComp<AIWhiteboard>().GetTimeAtWhichRobotGotOffCharger();
    const bool hasDrivenOff = FLT_GE(lastDriveOff, 0.0f);
    if ( !hasDrivenOff ) {
      // never driven off the charger, can't run
      return false;
    }

    const bool isRecent = FLT_LE(curTime, (lastDriveOff + _requiredRecentDriveOffCharger_sec));
    if ( !isRecent ) {
      // driven off, but not recently enough
      return false;
    }
  }

  // if there's a timer requiring a recent parent switch
  const bool requiresRecentParentSwitch = FLT_GE(_requiredRecentSwitchToParent_sec, 0.0);
  if ( requiresRecentParentSwitch ) {
    const float lastTime = 0.f;// robot.GetBehaviorManager().GetLastBehaviorChooserSwitchTime();
    const float changedAgoSecs = curTime - lastTime;
    const bool isSwitchRecent = FLT_LE(changedAgoSecs, _requiredRecentSwitchToParent_sec);
    if ( !isSwitchRecent ) {
      return false;
    }
  }

  //check if the behavior runs while in the air
  if(GetBEI().GetOffTreadsState() != OffTreadsState::OnTreads
      && !_operationModifiers.wantsToBeActivatedWhenOffTreads){
    return false;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  //check if the behavior can run while on the charger contacts (don't want most to run because they could damage
  //the robot by moving too much, and also will generally look dumb if they try to turn)
  if(robotInfo.IsOnChargerContacts() && !_operationModifiers.wantsToBeActivatedWhenOnCharger){
    return false;
  }

  //check if the behavior can handle holding a block
  if(robotInfo.GetCarryingComponent().IsCarryingObject() && !_operationModifiers.wantsToBeActivatedWhenCarryingObject){
    return false;
  }

  if(!CubeConnectionRequirementsMet()){
    return false;
  }

  for(auto& condition: _wantsToBeActivatedConditions){
    if(!condition->AreConditionsMet(GetBEI())){
      return false;
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::CubeConnectionRequirementsMet() const
{
  using requirements = BehaviorOperationModifiers::CubeConnectionRequirements;
  switch(_operationModifiers.cubeConnectionRequirements){
    case requirements::None:
    case requirements::OptionalLazy:
    case requirements::OptionalActive:
    {
      return true;
    }
    case requirements::RequiredManaged:
    {
#if ANKI_DEV_CHEATS
      // Runtime ancestor verification. TODO:(str) This should also be checked in unit tests
      bool foundRequiredAncestor = false;
      auto ancestorOperand = [&foundRequiredAncestor](const ICozmoBehavior& behavior){
        BehaviorOperationModifiers modifiers;
        behavior.GetBehaviorOperationModifiers(modifiers);
        foundRequiredAncestor = modifiers.ensuresCubeConnectionAtDelegation;
        return !foundRequiredAncestor;
      };
      ActiveBehaviorIterator& abi = GetBehaviorComp<ActiveBehaviorIterator>();
      abi.IterateActiveCozmoBehaviorsBackward(ancestorOperand);
      if(!ANKI_VERIFY(foundRequiredAncestor,
                      "ICozmoBehavior.MissingRequiredAncestor",
                      "Behavior %s is missing required ancestor %s, and cannot be activated",
                      GetDebugLabel().c_str(),
                      EnumToString(BehaviorClass::ConnectToCube))){
        return false;
      }
#endif // ANKI_DEV_CHEATS
      // NOTE: Fallthrough.
      // Both Required and Opportunistic require a currently active connection
    }
    case requirements::RequiredLazy:
    {
      if(!GetBEI().GetCubeConnectionCoordinator().IsConnectedToCube()){
        PRINT_NAMED_WARNING("ICozmoBehavior.MissingRequiredCubeConnection",
                            "Behavior %s specifies that a cube connection is required, but there is none active",
                            GetDebugLabel().c_str());
        return false;
      }
      return true;
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::ManageCubeConnectionSubscriptions(bool onActivated)
{
  using requirements = BehaviorOperationModifiers::CubeConnectionRequirements;
  bool shouldSubscribe = false;
  bool isConnected = GetBEI().GetCubeConnectionCoordinator().IsConnectedToCube();
  switch(_operationModifiers.cubeConnectionRequirements){
    case requirements::OptionalLazy:
    {
      shouldSubscribe = isConnected;
      break;
    }
    case requirements::OptionalActive:
    case requirements::RequiredLazy:
    case requirements::RequiredManaged:
    {
      shouldSubscribe = true;
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("ICozmoBehavior.ManageCubeConnectionSubscriptions.UnknownCubeConnectionRequirementType",
                        "Received unknown requirement type or None");
    }
  }

  if(shouldSubscribe){
    if(onActivated){
      GetBEI().GetCubeConnectionCoordinator().SubscribeToCubeConnection(this, _operationModifiers.connectToCubeInBackground);
    } else {
      GetBEI().GetCubeConnectionCoordinator().UnsubscribeFromCubeConnection(this);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& ICozmoBehavior::GetRNG() const {
  return GetBEI().GetRNG();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::UpdateInternal()
{
  //// Event handling
  // Call message handling convenience functions
  UpdateMessageHandlingHelpers();

  CheckDelegationCallbacks();

  // Tick behavior update
  BehaviorUpdate();


  if(IsActivated()){
    bool shouldCancelSelf = false;
    std::string baseDebugStr = "ICozmoBehavior.UpdateInternal.DataDefinedCancelSelf.";

    // Check whether we should cancel the behavior if control is no longer delegated
    if(_operationModifiers.behaviorAlwaysDelegates && !IsControlDelegated()){
      shouldCancelSelf = true;
      PRINT_NAMED_INFO((baseDebugStr + "ControlNotDelegated").c_str(),
                       "Behavior %s always delegates, so cancel self",
                       GetDebugLabel().c_str());
    }

    // Check wants to cancel self strategies
    for(auto& condition: _wantsToCancelSelfConditions){
      if(condition->AreConditionsMet(GetBEI())){
        shouldCancelSelf = true;
        PRINT_NAMED_INFO((baseDebugStr + "WantsToCancelSelfCondition").c_str(),
                         "Condition %s wants behavior %s to cancel itself",
                         condition->GetDebugLabel().c_str(),
                         GetDebugLabel().c_str());
      }
    }

    // Actually cancel the behavior
    if(shouldCancelSelf){
      if(GetBEI().HasDelegationComponent()){
        auto& delegationComponent = GetBEI().GetDelegationComponent();
        delegationComponent.CancelSelf(this);
        return;
      }
    }

  } // end IsActivated
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::UpdateMessageHandlingHelpers()
{
  const auto& stateChangeComp = GetBEI().GetBehaviorEventComponent();
  for(const auto& event: stateChangeComp.GetGameToEngineEvents()){
    // Handle specific callbacks
    auto iter = _gameToEngineTags.find(event.GetData().GetTag());
    if(iter != _gameToEngineTags.end()){
      AlwaysHandleInScope(event);
      if(IsActivated()){
        HandleWhileActivated(event);
      }else{
        HandleWhileInScopeButNotActivated(event);
      }
    }
  }

  for(const auto& event: stateChangeComp.GetEngineToGameEvents()){
    // Handle specific callbacks
    auto iter = _engineToGameTags.find(event.GetData().GetTag());
    if(iter != _engineToGameTags.end()){
      AlwaysHandleInScope(event);
      if(IsActivated()){
        HandleWhileActivated(event);
      }else{
        HandleWhileInScopeButNotActivated(event);
      }
    }
  }

  for(const auto& event: stateChangeComp.GetRobotToEngineEvents()){
    AlwaysHandleInScope(event);
    if(IsActivated()){
      HandleWhileActivated(event);
    }else{
      HandleWhileInScopeButNotActivated(event);
    }
  }
  
  for(const auto& event: stateChangeComp.GetAppToEngineEvents()){
    // Handle specific callbacks
    auto iter = _appToEngineTags.find(event.GetData().GetTag());
    if(iter != _appToEngineTags.end()){
      AlwaysHandleInScope(event);
      if(IsActivated()){
        HandleWhileActivated(event);
      }else{
        HandleWhileInScopeButNotActivated(event);
      }
    }
  }
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float ICozmoBehavior::GetActivatedDuration() const
{
  if (_isActivated)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceStarted = currentTime_sec - _activatedTime_s;
    return timeSinceStarted;
  }
  return 0.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::IsControlDelegated() const
{
  if(GetBEI().HasDelegationComponent()){
    auto& delegationComponent = GetBEI().GetDelegationComponent();
    return delegationComponent.IsControlDelegated(this);
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateNow(IActionRunner* action, RobotCompletedActionCallback callback)
{
  if(IsControlDelegated()){
    CancelDelegates();
  }
  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateNow(IActionRunner* action, ActionResultCallback callback)
{
  if(IsControlDelegated()){
    CancelDelegates();
  }
  return DelegateIfInControl(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateNow(IActionRunner* action, SimpleCallback callback)
{
  if(IsControlDelegated()){
    CancelDelegates();
  }
  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateNow(IBehavior* delegate, SimpleCallback callback)
{
  if(IsControlDelegated()){
    CancelDelegates();
  }
  return DelegateIfInControl(delegate, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback)
{
  if(!GetBEI().HasDelegationComponent()) {
    PRINT_NAMED_ERROR("ICozmoBehavior.DelegateIfInControl.NoDelegationComponent",
                      "Behavior %s attempted to start action while it did not have control of delegation",
                      GetDebugLabel().c_str());
    delete action;
    return false;
  }

  auto& delegationComponent = GetBEI().GetDelegationComponent();
  if(!delegationComponent.HasDelegator(this)){
    PRINT_NAMED_ERROR("ICozmoBehavior.DelegateIfInControl.ControlAlreadyDelegated",
                      "Behavior %s attempted to start action while it did not have control of delegation",
                      GetDebugLabel().c_str());
    delete action;
    return false;
  }

  auto& delegateWrapper = delegationComponent.GetDelegator(this);

  if( !IsActivated() ) {
    PRINT_NAMED_WARNING("ICozmoBehavior.DelegateIfInControl.Failure.NotRunning",
                        "Behavior '%s' can't start %s action because it is not running",
                        GetDebugLabel().c_str(), action->GetName().c_str());
    delete action;
    return false;
  }

  _delegationCallback = callback;
  _lastActionTag = action->GetTag();

  return delegateWrapper.Delegate(this,
                                  action);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, ActionResultCallback callback)
{
  return DelegateIfInControl(action,
                     [callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg.result);
                     });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, SimpleCallback callback)
{
  return DelegateIfInControl(action, [callback = std::move(callback)](ActionResult ret){ callback(); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IBehavior* delegate, BehaviorSimpleCallback callback)
{
  if((GetBEI().HasDelegationComponent()) &&
     !GetBEI().GetDelegationComponent().IsControlDelegated(this)) {
    auto& delegationComponent = GetBEI().GetDelegationComponent();

    if( delegationComponent.HasDelegator(this)) {
      delegationComponent.GetDelegator(this).Delegate(this, delegate);
      if(callback != nullptr){
        _delegationCallback = [callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg)
        {
          callback();
        };
      }
      return true;
    }
  }

  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ICozmoBehavior::FindBehavior( const std::string& behaviorIDStr ) const
{
  // first check anonymous behaviors
  ICozmoBehaviorPtr behavior = FindAnonymousBehaviorByName( behaviorIDStr );
  if( nullptr == behavior ) {
    // no match, try behavior IDs
    const BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString( behaviorIDStr );
    behavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( behaviorID );
    ANKI_VERIFY( behavior != nullptr,
                "ICozmoBehavior.FindBehavior.InvalidBehavior",
                "Behavior not found: %s", behaviorIDStr.c_str() );
  }
  return behavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ICozmoBehavior::FindAnonymousBehaviorByName(const std::string& behaviorName) const
{
  ICozmoBehaviorPtr foundBehavior = nullptr;

  auto it = _anonymousBehaviorMap.find(behaviorName);
  if (it != _anonymousBehaviorMap.end())
  {
    foundBehavior = it->second;
  }

  return foundBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::CheckDelegationCallbacks()
{
  // first call the behavior delegation callback if there is one
  if( IsActivated() &&
      !IsControlDelegated() &&
      _delegationCallback != nullptr) {
    // Note that the callback may itself call DelegateIfInControl and set _delegationCallback. Because of that, we create
    // a copy here so we can null out the member variable such that it can be re-set by callback (if desired)
    auto callback = _delegationCallback;
    _delegationCallback = nullptr;

    // check to see if an action completed and provide the real action completed message
    const auto& stateChangeComp = GetBEI().GetBehaviorEventComponent();
    const auto& actionsCompleted = stateChangeComp.GetActionsCompletedThisTick();
    for(const auto& entry: actionsCompleted){
      if(entry.idTag == _lastActionTag){
        callback(entry);
        return;
      }
    }

    // otherwise, it's a behavior callback so give it an empty action message
    ExternalInterface::RobotCompletedAction empty;
    callback(empty);
  }


}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::CancelDelegates(bool allowCallback)
{

  if( IsControlDelegated() ) {
    if(!allowCallback){
      _delegationCallback = nullptr;
    }
    if(GetBEI().HasDelegationComponent()){
      GetBEI().GetDelegationComponent().CancelDelegates(this);
      return true;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::CancelSelf()
{
  if(GetBEI().HasDelegationComponent()){
    auto& delegationComponent = GetBEI().GetDelegationComponent();
    delegationComponent.CancelSelf(this);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved, bool broadcastToGame) const
{
  /**auto robotExternalInterface = GetBEI().GetRobotExternalInterface().lock();
  if(broadcastToGame && (robotExternalInterface != nullptr)){
    robotExternalInterface->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(objectiveAchieved);
  }**/
  PRINT_CH_INFO("Behaviors", "ICozmoBehavior.BehaviorObjectiveAchieved", "Behavior:%s, Objective:%s", GetDebugLabel().c_str(), EnumToString(objectiveAchieved));
  // send das event
  Util::sInfoF("robot.freeplay_objective_achieved", {{DDATA, EnumToString(objectiveAchieved)}}, "%s", GetDebugLabel().c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartSetMotionProfile(const PathMotionProfile& motionProfile)
{
  ANKI_VERIFY(!_hasSetMotionProfile,
              "ICozmoBehavior.SmartSetMotionProfile.AlreadySet",
              "a profile was already set and not cleared");

  GetBEI().GetRobotInfo().GetPathComponent().SetCustomMotionProfile(motionProfile);
  _hasSetMotionProfile = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartClearMotionProfile()
{
  ANKI_VERIFY(_hasSetMotionProfile,
              "ICozmoBehavior.SmartClearMotionProfile.NotSet",
              "a profile was not set, so can't be cleared");
  GetBEI().GetRobotInfo().GetPathComponent().ClearCustomMotionProfile();
  _hasSetMotionProfile = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartLockTracks(u8 animationTracks, const std::string& who, const std::string& debugName)
{
  // Only lock the tracks if we haven't already locked them with that key
  const bool insertionSuccessfull = _lockingNameToTracksMap.insert(std::make_pair(who, animationTracks)).second;
  if(insertionSuccessfull){
    GetBEI().GetRobotInfo().GetMoveComponent().LockTracks(animationTracks, who, debugName);
    return true;
  }else{
    PRINT_NAMED_WARNING("ICozmoBehavior.SmartLockTracks.AttemptingToLockTwice", "Attempted to lock tracks with key named %s but key already exists", who.c_str());
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartUnLockTracks(const std::string& who)
{
  auto mapIter = _lockingNameToTracksMap.find(who);
  if(mapIter == _lockingNameToTracksMap.end()){
    PRINT_NAMED_WARNING("ICozmoBehavior.SmartUnlockTracks.InvalidUnlock", "Attempted to unlock tracks with key named %s but key not found", who.c_str());
    return false;
  }else{
    GetBEI().GetRobotInfo().GetMoveComponent().UnlockTracks(mapIter->second, who);
    mapIter = _lockingNameToTracksMap.erase(mapIter);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartSetCustomLightPattern(const ObjectID& objectID,
                                           const CubeAnimationTrigger& anim,
                                           const CubeLightAnimation::ObjectLights& modifier)
{
  if(std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID) == _customLightObjects.end()){
    GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(objectID, anim, nullptr, false, true, modifier);
    _customLightObjects.push_back(objectID);
    return true;
  }else{
    PRINT_NAMED_INFO("ICozmoBehavior.SmartSetCustomLightPattern.LightsAlreadySet",
                     "A custom light pattern has already been set on object %d", objectID.GetValue());
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartRemoveCustomLightPattern(const ObjectID& objectID,
                                              const std::vector<CubeAnimationTrigger>& anims)
{
  auto objectIter = std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID);
  if(objectIter != _customLightObjects.end()){
    for(const auto& anim : anims)
    {
      GetBEI().GetCubeLightComponent().StopLightAnimAndResumePrevious(anim, objectID);
    }
    _customLightObjects.erase(objectIter);
    return true;
  }else{
    PRINT_NAMED_INFO("ICozmoBehavior.SmartRemoveCustomLightPattern.LightsNotSet",
                        "No custom light pattern is set for object %d", objectID.GetValue());
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UserIntentPtr ICozmoBehavior::SmartActivateUserIntent(UserIntentTag tag)
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();

  // track the tag so that we can automatically deactivate it when this behavior deactivates
  _intentToDeactivate = tag;

  return uic.ActivateUserIntent(tag, GetDebugLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartDeactivateUserIntent()
{
  if( _intentToDeactivate != UserIntentTag::INVALID ) {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    uic.DeactivateUserIntent( _intentToDeactivate );
    _intentToDeactivate = UserIntentTag::INVALID;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartDisableEngineResponseToTriggerWord()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(!uic.IsTriggerWordDisabledByName(GetDebugLabel())){
    uic.DisableEngineResponseToTriggerWord(GetDebugLabel(), true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartEnableEngineResponseToTriggerWord()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsTriggerWordDisabledByName(GetDebugLabel())){
    uic.DisableEngineResponseToTriggerWord(GetDebugLabel(), false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartPushResponseToTriggerWord(const AnimationTrigger& getInAnimTrigger, 
                                                    const AudioEngine::Multiplexer::PostAudioEvent& postAudioEvent, 
                                                    StreamAndLightEffect streamAndLightEffect)
{
  _pushedCustomTriggerResponse = true;
  GetBehaviorComp<UserIntentComponent>().PushResponseToTriggerWord(GetDebugLabel(), getInAnimTrigger, postAudioEvent, streamAndLightEffect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartPushResponseToTriggerWord(const TriggerWordResponseData& newState)
{
  _pushedCustomTriggerResponse = true;
  GetBehaviorComp<UserIntentComponent>().PushResponseToTriggerWord(GetDebugLabel(), newState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartPopResponseToTriggerWord()
{
  if(_pushedCustomTriggerResponse){
    GetBehaviorComp<UserIntentComponent>().PopResponseToTriggerWord(GetDebugLabel());
  }
  _pushedCustomTriggerResponse = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartAlterStreamStateForCurrentResponse(const StreamAndLightEffect newEffect)
{
  if( _pushedCustomTriggerResponse ) {
    PRINT_NAMED_WARNING("ICozmoBehavior.SmartAlterStreamStateForCurrentResponse.AlreadyAltered",
                        "%s: already altered the stream state, and now doing so again (will pop old one)",
                        GetDebugLabel().c_str());
    SmartPopResponseToTriggerWord();
  }

  GetBehaviorComp<UserIntentComponent>().AlterStreamStateForCurrentResponse(GetDebugLabel(), newEffect);
  _pushedCustomTriggerResponse = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartRequestPowerSaveMode()
{
  auto& powerSaveManager = GetBehaviorComp<PowerStateManager>();
  
  if( !_powerSaveRequest.empty() ) {
    PRINT_NAMED_WARNING("ICozmoBehavior.SmartRequestPowerSaveMode.DuplicateRequest",
                        "%s: Power save mode already requested (with string '%s')",
                        GetDebugLabel().c_str(),
                        _powerSaveRequest.c_str());
    // remove the duplicate request to be safe
    SmartRemovePowerSaveModeRequest();
  }

  _powerSaveRequest = GetDebugLabel();
  powerSaveManager.RequestPowerSaveMode(_powerSaveRequest);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartRemovePowerSaveModeRequest()
{
  if( !_powerSaveRequest.empty() ) {
    auto& powerSaveManager = GetBehaviorComp<PowerStateManager>();
    powerSaveManager.RemovePowerSaveModeRequest(_powerSaveRequest);
    _powerSaveRequest.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartDisableKeepFaceAlive()
{
  if( ANKI_VERIFY( !_keepAliveDisabled,
                   "ICozmoBehavior.SmartDisableKeepFaceAlive.AlreadyDisabled",
                   "Behavior '%s' wants to disable face keep alive but it already has done so",
                   GetDebugLabel().c_str() ) ) {
    GetBEI().GetAnimationComponent().AddKeepFaceAliveDisableLock(GetDebugLabel());
    _keepAliveDisabled = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartReEnableKeepFaceAlive()
{
  if( ANKI_VERIFY( _keepAliveDisabled,
                   "ICozmoBehavior.SmartReEnableKeepFaceAlive.NotDisabled",
                   "Behavior '%s' wants to re-enable face keep alive but has not disabled it (through ICozmoBehavior)",
                   GetDebugLabel().c_str() ) ) {
    GetBEI().GetAnimationComponent().RemoveKeepFaceAliveDisableLock(GetDebugLabel());
    _keepAliveDisabled = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::PlayEmergencyGetOut(AnimationTrigger anim)
{
  auto& contComp = GetBEI().GetAIComponent().GetComponent<ContinuityComponent>();
  contComp.PlayEmergencyGetOut(anim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult ICozmoBehavior::UseSecondClosestPreActionPose(DriveToObjectAction* action,
                                                      ActionableObject* object,
                                                      std::vector<Pose3d>& possiblePoses,
                                                      bool& alreadyInPosition)
{
  auto result = action->GetPossiblePoses(object, possiblePoses, alreadyInPosition);
  if (result != ActionResult::SUCCESS) {
    return result;
  }

  auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  if( possiblePoses.size() > 1 && IDockAction::RemoveMatchingPredockPose(robotPose, possiblePoses ) ) {
    alreadyInPosition = false;
  }

  return ActionResult::SUCCESS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ICozmoBehavior::GetClassString(BehaviorClass behaviorClass) const
{
  return BehaviorTypesWrapper::BehaviorClassToString(behaviorClass);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SetDebugStateNameToWebViz() const
{
  const auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context != nullptr ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender("behaviors",
                                                                      context->GetWebService()) ) {
      webSender->Data()["debugState"] = _debugStateName;
    }
  }
}


} // namespace Vector
} // namespace Anki
