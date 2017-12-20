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

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "engine/actions/actionInterface.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorComponent/anonymousBehaviorFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudReceiver.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCloudIntentPending.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/cloudIntents.h"

#include "util/enums/stringToEnumMapper.hpp"
#include "util/fileUtils/fileUtils.h"
#include "util/math/numericCast.h"

#define LOG_CHANNEL    "Behaviors"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kBehaviorClassKey                 = "behaviorClass";
static const char* kBehaviorIDConfigKey              = "behaviorID";
static const char* kNeedsActionIDKey                 = "needsActionID";
static const char* kDisplayNameKey                   = "displayNameKey";

static const char* kRequiredUnlockKey                = "requiredUnlockId";
static const char* kRequiredSevereNeedsStateKey      = "requiredSevereNeedsState";
static const char* kRequiredDriveOffChargerKey       = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey          = "requiredRecentSwitchToParent_sec";
static const char* kExecutableBehaviorTypeKey        = "executableBehaviorType";
static const char* kAlwaysStreamlineKey              = "alwaysStreamline";
static const char* kWantsToRunStrategyConfigKey      = "wantsToRunStrategyConfig";
static const char* kRespondToCloudIntentKey          = "respondToCloudIntent";
static const std::string kIdleLockPrefix             = "Behavior_";

// Keys for loading in anonymous behaviors
static const char* kAnonymousBehaviorMapKey          = "anonymousBehaviors";
static const char* kAnonymousBehaviorName            = "behaviorName";
static const char* kAnonymousBehaviorParams          = "params";
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
NeedsActionId ICozmoBehavior::ExtractNeedsActionIDFromConfig(const Json::Value& config)
{
  const Json::Value& needsActionIDJson = config[kNeedsActionIDKey];
  const char* needsActionIDString = needsActionIDJson.isString() ? needsActionIDJson.asCString() : "";
  if (!needsActionIDString[0])
  {
    return NeedsActionId::NoAction;
  }
  return NeedsActionIdFromString(needsActionIDString);
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::ICozmoBehavior(const Json::Value& config)
: IBehavior(BehaviorTypesWrapper::BehaviorIDToString(ExtractBehaviorIDFromConfig(config)))
, _requiredProcess( AIInformationAnalysis::EProcess::Invalid )
, _lastRunTime_s(0.0f)
, _activatedTime_s(0.0f)
, _id(ExtractBehaviorIDFromConfig(config))
, _idString(BehaviorTypesWrapper::BehaviorIDToString(_id))
, _behaviorClassID(ExtractBehaviorClassFromConfig(config))
, _needsActionID(ExtractNeedsActionIDFromConfig(config))
, _executableType(BehaviorTypesWrapper::GetDefaultExecutableBehaviorType())
, _respondToCloudIntent(CloudIntent::Count)
, _requiredUnlockId( UnlockId::Count )
, _requiredSevereNeed( NeedId::Count )
, _requiredRecentDriveOffCharger_sec(-1.0f)
, _requiredRecentSwitchToParent_sec(-1.0f)
, _isActivated(false)
, _hasSetIdle(false)
{
  if(!ReadFromJson(config)){
    PRINT_NAMED_WARNING("ICozmoBehavior.ReadFromJson.Failed",
                        "Something went wrong reading from JSON");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::ReadFromJson(const Json::Value& config)
{
  JsonTools::GetValueOptional(config, kDisplayNameKey, _displayNameKey);

  // - - - - - - - - - -
  // Required unlock
  // - - - - - - - - - -
  const Json::Value& requiredUnlockJson = config[kRequiredUnlockKey];
  if ( !requiredUnlockJson.isNull() )
  {
    DEV_ASSERT(requiredUnlockJson.isString(), "ICozmoBehavior.ReadFromJson.NonStringUnlockId");
    
    // this is probably the only place where we need this, otherwise please refactor to proper header
    const UnlockId requiredUnlock = UnlockIdFromString(requiredUnlockJson.asString());
    if (requiredUnlock != UnlockId::Count) {
      PRINT_CH_DEBUG(LOG_CHANNEL, "ICozmoBehavior.ReadFromJson.RequiredUnlock",
                     "Behavior '%s' requires unlock '%s'",
                     GetIDStr().c_str(), requiredUnlockJson.asString().c_str());
      _requiredUnlockId = requiredUnlock;
    } else {
      PRINT_NAMED_ERROR("ICozmoBehavior.ReadFromJson.InvalidUnlockId", "Could not convert string to unlock id '%s'",
        requiredUnlockJson.asString().c_str());
    }
  }

  // - - - - - - - - - - - - - -
  // Required severe needs state
  // - - - - - - - - - - - - - -
  const Json::Value& requiredSevereNeedJson = config[kRequiredSevereNeedsStateKey];
  if( !requiredSevereNeedJson.isNull() ) {
    DEV_ASSERT(requiredSevereNeedJson.isString(), "ICozmoBehavior.ReadFromJson.NonStringRequiredNeedsState");

    _requiredSevereNeed = NeedIdFromString(requiredSevereNeedJson.asString());
    ANKI_VERIFY(_requiredSevereNeed != NeedId::Count,
                "ICozmoBehavior.ReadFromJson.ConfigError.InvalidRequiredSevereNeed",
                "%s: Required severe need '%s' converted to Count. This field is optional",
                GetIDStr().c_str(),
                requiredSevereNeedJson.asCString());
  }
  
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
  
  if(config.isMember(kWantsToRunStrategyConfigKey)){
    _wantsToBeActivatedStrategies.push_back(
      BEIConditionFactory::CreateBEICondition( config[kWantsToRunStrategyConfigKey] ) );
  }

  if(config.isMember(kAnonymousBehaviorMapKey)){
    _anonymousBehaviorMapConfig = config[kAnonymousBehaviorMapKey];
  }    
    
  if(config.isMember(kRespondToCloudIntentKey)){
    _respondToCloudIntent = CloudIntentFromString(config[kRespondToCloudIntentKey].asCString());
  }

  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::~ICozmoBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  // Assumes there's only one external interface being passed in from AIComponent
  _behaviorExternalInterface = &behaviorExternalInterface;
  assert(_behaviorExternalInterface);
  
  {
    for( auto& strategy : _wantsToBeActivatedStrategies ) {
      strategy->Init(behaviorExternalInterface);
    }

    if(_respondToCloudIntent != CloudIntent::Count){
      Json::Value config = ConditionCloudIntentPending::GenerateCloudIntentPendingConfig(_respondToCloudIntent);
      IBEIConditionPtr strategy(BEIConditionFactory::CreateBEICondition(config));
      strategy->Init(behaviorExternalInterface);
      _wantsToBeActivatedStrategies.push_back(strategy);
    }

  }

  if(!_anonymousBehaviorMapConfig.empty()){
    AnonymousBehaviorFactory factory(behaviorExternalInterface.GetBehaviorContainer()); 
    for(auto& entry: _anonymousBehaviorMapConfig){
      const std::string debugStr = "ICozmoBehavior.ReadFromJson.";
      
      const std::string behaviorName = JsonTools::ParseString(entry, kAnonymousBehaviorName, debugStr + "BehaviorNameMissing");      
      const BehaviorClass behaviorClass = BehaviorTypesWrapper::BehaviorClassFromString(
        JsonTools::ParseString(entry, kBehaviorClassKey, debugStr + "BehaviorClassMissing"));
      Json::Value params;
      if(entry.isMember(kAnonymousBehaviorParams)){
        params = entry[kAnonymousBehaviorParams];
      }

      // check for duplicate behavior names since maps require a unique key
      auto it = _anonymousBehaviorMap.find(behaviorName);
      if(it == _anonymousBehaviorMap.end()){
        auto resultPair = _anonymousBehaviorMap.insert(std::make_pair(behaviorName,
                                                       factory.CreateBehavior(behaviorClass, params)));

        DEV_ASSERT_MSG(resultPair.first->second != nullptr, "ICozmoBehavior.InitInternal.FailedToAllocateAnonymousBehavior",
                       "Failed to allocate new anonymous behavior (%s) within behavior %s",
                       behaviorName.c_str(),
                       GetIDStr().c_str());

        // we need to initlaize the anon behaviors as well
        resultPair.first->second->Init(behaviorExternalInterface);
      }
      else
      {
        PRINT_NAMED_ERROR("ICozmoBehavior.InitInternal.DuplicateAnonymousBehaviorName",
                          "Duplicate anonymous behavior name (%s) found for behavior '%s'",
                          behaviorName.c_str(),
                          GetIDStr().c_str());
      }
    }
  }


  // Allow internal init to happen before subscribing to tags in case additional
  // tags are added
  InitBehavior(behaviorExternalInterface);

  ///////
  //// Subscribe to tags
  ///////
  
  for(auto tag : _gameToEngineTags) {
    if(ANKI_VERIFY(_behaviorExternalInterface != nullptr,
                   "ICozmoBehavior.SubscribeToTag.MissingExternalInterface",
                   "")){
      _behaviorExternalInterface->GetStateChangeComponent().SubscribeToTags(this, {tag});
    }
  }
  
  for(auto tag : _engineToGameTags) {
    if(ANKI_VERIFY(_behaviorExternalInterface != nullptr,
                   "ICozmoBehavior.SubscribeToTag.MissingExternalInterface",
                   "")){
      _behaviorExternalInterface->GetStateChangeComponent().SubscribeToTags(this, {tag});
    }
  }
  
  for(auto tag: _robotToEngineTags) {
    behaviorExternalInterface.GetStateChangeComponent().SubscribeToTags(this,{tag});
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  _gameToEngineTags = std::move(tags);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  _engineToGameTags = std::move(tags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SubscribeToTags(std::set<RobotInterface::RobotToEngineTag> &&tags)
{
  _robotToEngineTags = std::move(tags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Init").c_str(), "Starting...");
  
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
                        GetIDStr().c_str(), robot.GetActionList().GetQueueLength(0));
  }**/
  
  _isActivated = true;
  _stopRequestedAfterAction = false;
  _actionCallback = nullptr;
  _behaviorDelegateCallback = nullptr;
  _activatedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _hasSetIdle = false;
  _startCount++;
  
  // Clear cloud intent if responding to it
  if(_respondToCloudIntent != CloudIntent::Count){
    behaviorExternalInterface.GetAIComponent().GetBehaviorComponent().
                 GetCloudReceiver().ClearIntentIfPending(_respondToCloudIntent);
  }

  OnBehaviorActivated(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnEnteredActivatableScopeInternal()
{
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid ){
    auto& infoProcessor = _behaviorExternalInterface->GetAIComponent().GetAIInformationAnalyzer();
    infoProcessor.AddEnableRequest(_requiredProcess, GetIDStr().c_str());
  }

  for( auto& strategy : _wantsToBeActivatedStrategies ) {
    strategy->Reset(*_behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnLeftActivatableScopeInternal()
{
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid ){
    auto& infoProcessor = _behaviorExternalInterface->GetAIComponent().GetAIInformationAnalyzer();
    infoProcessor.RemoveEnableRequest(_requiredProcess, GetIDStr().c_str());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Stop").c_str(), "Stopping...");

  // If the behavior delegated off to a helper, stop that first
  if(!_currentHelperHandle.expired()){
    StopHelperWithoutCallback();
  }
  
  _isActivated = false;
  OnBehaviorDeactivated(behaviorExternalInterface);
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  CancelDelegates(false);
  
  if(_hasSetIdle){
    SmartRemoveIdleAnimation(*_behaviorExternalInterface);
  }
  
  // clear the path component motion profile if it was set by the behavior
  if( _hasSetMotionProfile ) {
    SmartClearMotionProfile();
  }

  // Unlock any tracks which the behavior hasn't had a chance to unlock
  for(const auto& entry: _lockingNameToTracksMap){
    behaviorExternalInterface.GetRobotInfo().GetMoveComponent().UnlockTracks(entry.second, entry.first);
  }
  
  
  _lockingNameToTracksMap.clear();
  _customLightObjects.clear();
  
  DEV_ASSERT(_smartLockIDs.empty(), "ICozmoBehavior.Stop.DisabledReactionsNotEmpty");

  OnCozmoBehaviorDeactivated(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::StopOnNextActionComplete()
{
  if( !_stopRequestedAfterAction ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".StopOnNextActionComplete").c_str(),
                  "Behavior has been asked to stop on the next complete action");
  }
  
  // clear the callback and don't let any new actions start
  _stopRequestedAfterAction = true;
  _actionCallback = nullptr;
  _behaviorDelegateCallback = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(WantsToBeActivatedBase(behaviorExternalInterface)){
    return WantsToBeActivatedBehavior(behaviorExternalInterface);
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::WantsToBeActivatedBase(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // Some reaction trigger strategies allow behaviors to interrupt themselves.
  DEV_ASSERT(!IsActivated(), "ICozmoBehavior.WantsToBeActivatedCalledOnRunningBehavior");
  if (IsActivated()) {
    PRINT_CH_DEBUG("Behaviors", "ICozmoBehavior.WantsToBeActivatedBase", "Behavior %s is already running", GetIDStr().c_str());
    return true;
  }
  
  // check if required processes are running
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid )
  {
    const bool isProcessOn = behaviorExternalInterface.GetAIComponent().GetAIInformationAnalyzer().IsProcessRunning(_requiredProcess);
    if ( !isProcessOn ) {
      PRINT_NAMED_ERROR("ICozmoBehavior.WantsToBeActivated.RequiredProcessNotFound",
        "Required process '%s' is not enabled for '%s'",
        AIInformationAnalysis::StringFromEProcess(_requiredProcess),
        GetIDStr().c_str());
      return false;
    }
  }

  // check if a severe needs state is required
  if( _requiredSevereNeed != NeedId::Count &&
      _requiredSevereNeed != behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression() ) {
    // not in the correct state
    return false;
  }

  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // first check the unlock
  if ( _requiredUnlockId != UnlockId::Count )
  {
    if(_behaviorExternalInterface->HasProgressionUnlockComponent()){
      // ask progression component if the unlockId is currently unlocked
      auto& progressionUnlockComp = behaviorExternalInterface.GetProgressionUnlockComponent();
      const bool forFreeplay = true;
      const bool isUnlocked = progressionUnlockComp.IsUnlocked(_requiredUnlockId,
                                                               forFreeplay );
      if ( !isUnlocked ) {
        return false;
      }
    }
  }
  
  // if there's a timer requiring a recent drive off the charger, check with whiteboard
  const bool requiresRecentDriveOff = FLT_GE(_requiredRecentDriveOffCharger_sec, 0.0f);
  if ( requiresRecentDriveOff )
  {
    const float lastDriveOff = behaviorExternalInterface.GetAIComponent().GetWhiteboard().GetTimeAtWhichRobotGotOffCharger();
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
  if(behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads
     && !ShouldRunWhileOffTreads()){
    return false;
  }

  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  //check if the behavior can run from the charger platform (don't want most to run because they could damage
  //the robot by moving too much, and also will generally look dumb if they try to turn)
  if(robotInfo.IsOnChargerPlatform() && !ShouldRunWhileOnCharger()) {
    return false;
  }
  
  //check if the behavior can handle holding a block
  if(robotInfo.GetCarryingComponent().IsCarryingObject() && !CarryingObjectHandledInternally()){
    return false;
  }
  
  for(auto& strategy: _wantsToBeActivatedStrategies){
    if(!strategy->AreConditionsMet(behaviorExternalInterface)){
      return false;
    }
  }
     
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& ICozmoBehavior::GetRNG() const {
  return _behaviorExternalInterface->GetRNG();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // fist call the behavior delegation callback if there is one
  if( IsActivated() &&
      !IsControlDelegated() &&
      _behaviorDelegateCallback != nullptr ) {

    // make local copy to avoid any issues with the callback delegating to another behavior
    auto callback = _behaviorDelegateCallback;
    _behaviorDelegateCallback = nullptr;
    callback(behaviorExternalInterface);
  }


  //// Event handling
  // Call message handling convenience functions
  //////
  
  const auto& stateChangeComp = behaviorExternalInterface.GetStateChangeComponent();
  const auto& actionsCompleted = stateChangeComp.GetActionsCompletedThisTick();
  for(auto& entry: actionsCompleted){
    if(entry.idTag == _lastActionTag){
      HandleActionComplete(entry);
    }
  }
  
  for(const auto& event: stateChangeComp.GetGameToEngineEvents()){
    // Handle specific callbacks
    auto iter = _gameToEngineTags.find(event.GetData().GetTag());
    if(iter != _gameToEngineTags.end()){
      AlwaysHandle(event, behaviorExternalInterface);
      if(IsActivated()){
        HandleWhileActivated(event, behaviorExternalInterface);
      }else{
        HandleWhileInScopeButNotActivated(event, behaviorExternalInterface);
      }
    }
  }
  
  for(const auto& event: stateChangeComp.GetEngineToGameEvents()){
    // Handle specific callbacks
    auto iter = _engineToGameTags.find(event.GetData().GetTag());
    if(iter != _engineToGameTags.end()){      
      AlwaysHandle(event, behaviorExternalInterface);
      if(IsActivated()){
        HandleWhileActivated(event, behaviorExternalInterface);
      }else{
        HandleWhileInScopeButNotActivated(event, behaviorExternalInterface);
      }
    }
  }
  
  for(const auto& event: stateChangeComp.GetRobotToEngineEvents()){
    AlwaysHandle(event, behaviorExternalInterface);
    if(IsActivated()){
      HandleWhileActivated(event, behaviorExternalInterface);
    }else{
      HandleWhileInScopeButNotActivated(event, behaviorExternalInterface);
    }
  }
  
  //////
  //// end Event handling
  //////

  if(IsActivated()){
    if(!IsActing()){
      if(_stopRequestedAfterAction) {
        // we've been asked to stop, so do that
        if(behaviorExternalInterface.HasDelegationComponent()){
          auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
          delegationComponent.CancelSelf(this);
        }
      }
    }
  }

  BehaviorUpdate(behaviorExternalInterface);

  if(IsActivated()){
    if(ShouldCancelWhenInControl() && !IsControlDelegated()){
      if(behaviorExternalInterface.HasDelegationComponent()){
        auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
        delegationComponent.CancelSelf(this);
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
  if(_behaviorExternalInterface->HasDelegationComponent()){
    auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();
    return delegationComponent.IsControlDelegated(this);
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::IsActing() const
{
  if(_behaviorExternalInterface->HasDelegationComponent()){
    auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();
    return delegationComponent.IsActing(this);
  }
  
  return false;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback)
{
  if(!_behaviorExternalInterface->HasDelegationComponent()) {
    PRINT_NAMED_ERROR("ICozmoBehavior.DelegateIfInControl.NoDelegationComponent",
                      "Behavior %s attempted to start action while it did not have control of delegation",
                      GetIDStr().c_str());
    delete action;
    return false;
  }
  
  auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();
  if(!delegationComponent.HasDelegator(this)){
    PRINT_NAMED_ERROR("ICozmoBehavior.DelegateIfInControl.NoDelegator",
                      "Behavior %s attempted to start action while it did not have control of delegation",
                      GetIDStr().c_str());
    delete action;
    return false;
  }
  
  auto& delegateWrapper = delegationComponent.GetDelegator(this);

  
  // needed for StopOnNextActionComplete to work properly, don't allow starting new actions if we've requested
  // the behavior to stop
  if( _stopRequestedAfterAction ) {
    delete action;
    return false;
  }
  
  if( !IsActivated() ) {
    PRINT_NAMED_WARNING("ICozmoBehavior.DelegateIfInControl.Failure.NotRunning",
                        "Behavior '%s' can't start %s action because it is not running",
                        GetIDStr().c_str(), action->GetName().c_str());
    delete action;
    return false;
  }

  if( IsActing() ) {
    PRINT_NAMED_WARNING("ICozmoBehavior.DelegateIfInControl.Failure.AlreadyActing",
                        "Behavior '%s' can't start %s action because it is already running an action in state %s",
                        GetIDStr().c_str(), action->GetName().c_str(),
                        GetDebugStateName().c_str());
    delete action;
    return false;
  }

  _actionCallback = callback;
  _lastActionTag = action->GetTag();
    
  return delegateWrapper.Delegate(this,
                                  action);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionWithExternalInterfaceCallback callback)
{
  return DelegateIfInControl(action,
                     [this, callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg, *_behaviorExternalInterface);
                     });
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
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, ActionResultWithRobotCallback callback)
{
  return DelegateIfInControl(action,
                     [this, callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg.result, *_behaviorExternalInterface);
                     });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, SimpleCallback callback)
{
  return DelegateIfInControl(action, [callback = std::move(callback)](ActionResult ret){ callback(); });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, SimpleCallbackWithRobot callback)
{
  return DelegateIfInControl(action, [this, callback = std::move(callback)](ActionResult ret){ callback(*_behaviorExternalInterface); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface, IBehavior* delegate)
{
  if((_behaviorExternalInterface->HasDelegationComponent()) &&
     !behaviorExternalInterface.GetDelegationComponent().IsControlDelegated(this)) {
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
    
    if( delegationComponent.HasDelegator(this)) {
      delegationComponent.GetDelegator(this).Delegate(this, delegate);
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface,
                                         IBehavior* delegate,
                                         BehaviorSimpleCallbackWithExternalInterface callback)
{
  // TODO:(bn) unit test this!!!

  if( DelegateIfInControl(behaviorExternalInterface, delegate) ) {
    _behaviorDelegateCallback = callback;
    return true;
  }
  else {
    // couldn't delegate (for whatever reason)
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::DelegateNow(BehaviorExternalInterface& behaviorExternalInterface, IBehavior* delegate)
{
  if(_behaviorExternalInterface->HasDelegationComponent()) {
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
    if( delegationComponent.IsControlDelegated(this) ) {
      delegationComponent.CancelDelegates(this);
    }
    
    DEV_ASSERT(!delegationComponent.IsControlDelegated(this), "IBehavior.DelegateNow.CanceledButStillNotInControl");
    
    if(delegationComponent.HasDelegator(this)) {
      auto& delegator = delegationComponent.GetDelegator(this);
      delegator.Delegate(this, delegate);
      return true;
    }
  }
  
  return false;
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
void ICozmoBehavior::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
{
  if( _actionCallback ) {

    // Note that the callback may itself call DelegateIfInControl and set _actionCallback. Because of that, we create
    // a copy here so we can null out the member variable such that it can be re-set by callback (if desired)
    auto callback = _actionCallback;
    _actionCallback = nullptr;
    
    callback(msg);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::CancelDelegates(bool allowCallback, bool allowHelperToContinue)
{
  if( !allowHelperToContinue ) {
    // stop the helper first. This is generally what we want, because if someone else is stopping an action,
    // the helper likely won't know how to respond. Note that helpers can't call StopActing
    if( !_currentHelperHandle.expired() ) {
      PRINT_CH_INFO("Behaviors", (GetIDStr() + ".StopActing.WithoutCallback.StopHelper").c_str(),
                    "Stopping behavior helper because action stopped without callback");
    }
    StopHelperWithoutCallback();
  }    
  
  if( IsControlDelegated() ) {
    if(!allowCallback){
      _actionCallback = nullptr;
      _behaviorDelegateCallback = nullptr;
    }
    if(_behaviorExternalInterface->HasDelegationComponent()){
      _behaviorExternalInterface->GetDelegationComponent().CancelDelegates(this);
      return true;
    }
  }

  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::CancelSelf()
{
  if(_behaviorExternalInterface->HasDelegationComponent()){
    auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();
    delegationComponent.CancelSelf(this);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved, bool broadcastToGame) const
{
  /**auto robotExternalInterface = _behaviorExternalInterface->GetRobotExternalInterface().lock();
  if(broadcastToGame && (robotExternalInterface != nullptr)){
    robotExternalInterface->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(objectiveAchieved);
  }**/
  PRINT_CH_INFO("Behaviors", "ICozmoBehavior.BehaviorObjectiveAchieved", "Behavior:%s, Objective:%s", GetIDStr().c_str(), EnumToString(objectiveAchieved));
  // send das event
  Util::sEventF("robot.freeplay_objective_achieved", {{DDATA, EnumToString(objectiveAchieved)}}, "%s", GetIDStr().c_str());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::NeedActionCompleted(NeedsActionId needsActionId)
{
  if (needsActionId == NeedsActionId::NoAction)
  {
    needsActionId = _needsActionID;
  }
  
  if(_behaviorExternalInterface->HasNeedsManager()){
    auto& needsManager = _behaviorExternalInterface->GetNeedsManager();
    needsManager.RegisterNeedsActionCompleted(needsActionId);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartPushIdleAnimation(BehaviorExternalInterface& behaviorExternalInterface, AnimationTrigger animation)
{
  if(ANKI_VERIFY(!_hasSetIdle,
                 "ICozmoBehavior.SmartPushIdleAnimation.IdleAlreadySet",
                 "Behavior %s has already set an idle animation",
                 GetIDStr().c_str())){
    // TODO: Restore idle animations (VIC-366)
    //robot.GetAnimationStreamer().PushIdleAnimation(animation, kIdleLockPrefix + GetIDStr());
    _hasSetIdle = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartRemoveIdleAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(ANKI_VERIFY(_hasSetIdle,
                 "ICozmoBehavior.SmartRemoveIdleAnimation.NoIdleSet",
                 "Behavior %s is trying to remove an idle, but none is currently set",
                 GetIDStr().c_str())){
    // TODO: Restore idle animations (VIC-366)
    //robot.GetAnimationStreamer().RemoveIdleAnimation(kIdleLockPrefix + GetIDStr());
    _hasSetIdle = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartSetMotionProfile(const PathMotionProfile& motionProfile)
{
  ANKI_VERIFY(!_hasSetMotionProfile,
              "ICozmoBehavior.SmartSetMotionProfile.AlreadySet",
              "a profile was already set and not cleared");
  
  _behaviorExternalInterface->GetRobotInfo().GetPathComponent().SetCustomMotionProfile(motionProfile);
  _hasSetMotionProfile = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartClearMotionProfile()
{
  ANKI_VERIFY(_hasSetMotionProfile,
              "ICozmoBehavior.SmartClearMotionProfile.NotSet",
              "a profile was not set, so can't be cleared");
  _behaviorExternalInterface->GetRobotInfo().GetPathComponent().ClearCustomMotionProfile();
  _hasSetMotionProfile = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartLockTracks(u8 animationTracks, const std::string& who, const std::string& debugName)
{
  // Only lock the tracks if we haven't already locked them with that key
  const bool insertionSuccessfull = _lockingNameToTracksMap.insert(std::make_pair(who, animationTracks)).second;
  if(insertionSuccessfull){
    _behaviorExternalInterface->GetRobotInfo().GetMoveComponent().LockTracks(animationTracks, who, debugName);
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
    _behaviorExternalInterface->GetRobotInfo().GetMoveComponent().UnlockTracks(mapIter->second, who);
    mapIter = _lockingNameToTracksMap.erase(mapIter);
    return true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::SmartSetCustomLightPattern(const ObjectID& objectID,
                                           const CubeAnimationTrigger& anim,
                                           const ObjectLights& modifier)
{
  if(std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID) == _customLightObjects.end()){
    _behaviorExternalInterface->GetCubeLightComponent().PlayLightAnim(objectID, anim, nullptr, true, modifier);
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
      _behaviorExternalInterface->GetCubeLightComponent().StopLightAnimAndResumePrevious(anim, objectID);
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
bool ICozmoBehavior::SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                      HelperHandle handleToRun,
                                      SimpleCallbackWithRobot successCallback,
                                      SimpleCallbackWithRobot failureCallback)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".SmartDelegateToHelper").c_str(),
                "Behavior requesting to delegate to helper %s", handleToRun->GetName().c_str());

  if(!_currentHelperHandle.expired()){
   PRINT_NAMED_WARNING("ICozmoBehavior.SmartDelegateToHelper",
                       "Attempted to start a handler while handle already running, stopping running helper");
   StopHelperWithoutCallback();
  }
  
  if(!_behaviorExternalInterface->HasDelegationComponent()){
    PRINT_NAMED_ERROR("ICozmoBehavior.SmartDelegateToHelper.NotInControlOfDelegationComponent",
                      "Behavior %s attempted to delegate while not in control",
                      GetIDStr().c_str());
    return false;
  }
  
  auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();

  
  if(!delegationComponent.HasDelegator(this)){
    PRINT_NAMED_ERROR("ICozmoBehavior.SmartDelegateToHelper.NotInControlOfDelegator",
                      "Behavior %s attempted to delegate while not in control",
                      GetIDStr().c_str());
    return false;
  }
  auto& delegateWrapper = delegationComponent.GetDelegator(this);

  
  // A bit of a hack while BSM is still under construction - essentially IsControlDelegated
  // is now overloaded for both helpers and actions, but DelegateIfInControl needs to be able
  // to distinguish the same IsActing used to indicate only actions - assigning
  // this tmp handle indicates to behaviors that they've "delegated" and should allow
  // helpers to queue actions - but if the delegation fails this tmp handle will fall
  // out of scope as soon as this function ends so that _currentHelperHandle becomes
  // invalid again
  HelperHandle tmpHandle = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().
                                        GetBehaviorHelperFactory().CreatePlaceBlockHelper(*_behaviorExternalInterface, *this);
  _currentHelperHandle = tmpHandle;
  
  const bool delegateSuccess = delegateWrapper.Delegate(this,
                                                        behaviorExternalInterface,
                                                        handleToRun,
                                                        successCallback,
                                                        failureCallback);

  if( delegateSuccess ) {
    _currentHelperHandle = handleToRun;
  }
  else {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + "SmartDelegateToHelper.Failed").c_str(),
                  "Failed to delegate to helper");
  }
  
  return delegateSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperFactory& ICozmoBehavior::GetBehaviorHelperFactory()
{
  return _behaviorExternalInterface->GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehavior::StopHelperWithoutCallback()
{
  bool handleStopped = false;
  auto handle = _currentHelperHandle.lock();
  if( handle ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".SmartStopHelper").c_str(),
                  "Behavior stopping its helper");
    if(_behaviorExternalInterface->HasDelegationComponent()){
      auto& delegationComponent = _behaviorExternalInterface->GetDelegationComponent();
      delegationComponent.CancelDelegates(this);
      handleStopped = true;
    }
  }
  
  return handleStopped;
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

  auto& robotPose = _behaviorExternalInterface->GetRobotInfo().GetPose();
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

  
} // namespace Cozmo
} // namespace Anki
