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
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
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
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/userIntent.h"

#include "util/enums/stringToEnumMapper.hpp"
#include "util/fileUtils/fileUtils.h"
#include "util/math/numericCast.h"

#define LOG_CHANNEL    "Behaviors"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kBehaviorClassKey                 = "behaviorClass";
static const char* kBehaviorIDConfigKey              = "behaviorID";

static const char* kRequiredUnlockKey                = "requiredUnlockId";
static const char* kRequiredDriveOffChargerKey       = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey          = "requiredRecentSwitchToParent_sec";
static const char* kExecutableBehaviorTypeKey        = "executableBehaviorType";
static const char* kAlwaysStreamlineKey              = "alwaysStreamline";
static const char* kWantsToBeActivatedCondConfigKey  = "wantsToBeActivatedCondition";
static const char* kRespondToUserIntentsKey          = "respondToUserIntents";
static const char* kClaimUserIntentDataKey           = "claimUserIntentData";
static const char* kRespondToTriggerWordKey          = "respondToTriggerWord";
static const char* kResetTimersKey                   = "resetTimers";
static const std::string kIdleLockPrefix             = "Behavior_";

// Keys for loading in anonymous behaviors
static const char* kAnonymousBehaviorMapKey          = "anonymousBehaviors";
static const char* kAnonymousBehaviorName            = "behaviorName";
  
static const char* kBehaviorDebugLabel               = "debugLabel";
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
  // this will be unique per BehaviorID instead of per "kBehaviorDebugLabel," but theyre usually the same
  static std::unordered_map<BehaviorID, unsigned int> counts;
  const auto index = counts[behaviorID]++; // and post-increment
  if( index > 0 ) {
    ret += std::to_string(index);
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::ICozmoBehavior(const Json::Value& config)
: IBehavior( MakeUniqueDebugLabelFromConfig( config ) )
, _requiredProcess( AIInformationAnalysis::EProcess::Invalid )
, _lastRunTime_s(0.0f)
, _activatedTime_s(0.0f)
, _id(ExtractBehaviorIDFromConfig(config))
, _behaviorClassID(ExtractBehaviorClassFromConfig(config))
, _executableType(BehaviorTypesWrapper::GetDefaultExecutableBehaviorType())
, _claimUserIntentData( true )
, _respondToTriggerWord( false )
, _requiredUnlockId( UnlockId::Count )
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
                     GetDebugLabel().c_str(), requiredUnlockJson.asString().c_str());
      _requiredUnlockId = requiredUnlock;
    } else {
      PRINT_NAMED_ERROR("ICozmoBehavior.ReadFromJson.InvalidUnlockId", "Could not convert string to unlock id '%s'",
        requiredUnlockJson.asString().c_str());
    }
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
  
  if(config.isMember(kWantsToBeActivatedCondConfigKey)){
    _wantsToBeActivatedConditions.push_back(
      BEIConditionFactory::CreateBEICondition( config[kWantsToBeActivatedCondConfigKey], GetDebugLabel() ) );
  }

  if(config.isMember(kAnonymousBehaviorMapKey)){
    _anonymousBehaviorMapConfig = config[kAnonymousBehaviorMapKey];
  }    
  
  if(config.isMember(kRespondToUserIntentsKey)){
    // create a ConditionUserIntentPending based on the config json
    Json::Value json = ConditionUserIntentPending::GenerateConfig( config[kRespondToUserIntentsKey] );
    // another shared_ptr is kept outside of the regular list _wantsToBeActivatedConditions to avoid casting
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>( json );
    _respondToUserIntent->SetOwnerDebugLabel( GetDebugLabel() );
    _wantsToBeActivatedConditions.push_back( _respondToUserIntent );
    
    _claimUserIntentData = config.get( kClaimUserIntentDataKey, true ).asBool();
  }
  
  _respondToTriggerWord = config.get(kRespondToTriggerWordKey, false).asBool();
  
  JsonTools::GetVectorOptional(config, kResetTimersKey, _resetTimers);
  for( const auto& timerName : _resetTimers ) {
    ANKI_VERIFY( BehaviorTimerManager::IsValidName( timerName ),
                 "ICozmoBehavior.ReadFromJson.InvalidTimer",
                 "Timer '%s' is invalid",
                 timerName.c_str() );
  }

  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::~ICozmoBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::InitInternal()
{
  _initHasBeenCalled = true;

  {
    for( auto& strategy : _wantsToBeActivatedConditions ) {
      strategy->Init(GetBEI());
    }

    if( _respondToTriggerWord ){
      IBEIConditionPtr strategy(BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel()));
      strategy->Init(GetBEI());
      _wantsToBeActivatedConditions.push_back(strategy);
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

          // we need to initlaize the anon behaviors as well
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
  _dontActivateCoordinator = coordinatorName;
  _tickDontActivateSetFor = BaseStationTimer::getInstance()->GetTickCount();
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
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>();
    _respondToUserIntent->SetOwnerDebugLabel( GetDebugLabel() );
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
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>();
    _respondToUserIntent->SetOwnerDebugLabel( GetDebugLabel() );
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
    _respondToUserIntent = std::make_shared<ConditionUserIntentPending>();
    _respondToUserIntent->SetOwnerDebugLabel( GetDebugLabel() );
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
  
  _isActivated = true;
  _delegationCallback = nullptr;
  _activatedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _hasSetIdle = false;
  _startCount++;
  
  // Clear user intent if responding to it. Since _respondToUserIntent is initialized, this
  // behavior has a BEI condition that won't be true unless the specific user intent is pending.
  // If this behavior was activated, then it must have been true, so it suffices to check if it's
  // initialized instead of evaluating it here.
  if( _respondToUserIntent != nullptr ) {
    auto tag = _respondToUserIntent->GetUserIntentTagSelected();
    if( tag != USER_INTENT(INVALID) ) {
      auto& uic = GetBEI().GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
      if( _claimUserIntentData ) {
        // u now pwn it
        _pendingIntent.reset( uic.ClearUserIntentWithOwnership( tag ) );
      } else {
        uic.ClearUserIntentWithPreservation( tag, GetID() );
      }
    } else {
      // this can happen in tests
      PRINT_NAMED_WARNING("ICozmoBehavior.OnActivatedInternal.InvalidTag",
                          "_respondToUserIntent said it was responding to the INVALID tag" );
      _pendingIntent.reset();
    }
  }
  
  // Clear trigger word if responding to it
  if( _respondToTriggerWord ) {
    GetBEI().GetAIComponent().GetBehaviorComponent().GetUserIntentComponent().ClearPendingTriggerWord();
  }

  // Handle Vision Mode Subscriptions
  if(!_operationModifiers.visionModesForActiveScope->empty()){
    GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, 
      *_operationModifiers.visionModesForActiveScope);
  }

  // Manage state for any IBEIConditions used by this Behavior
  // Conditions may not be evaluted when the behavior is Active
  for(auto& strategy: _wantsToBeActivatedConditions){
    strategy->SetActive(GetBEI(), false);
  }
  
  // reset any timers
  for( const auto& timerName : _resetTimers ) {
    auto timer = BehaviorTimerManager::BehaviorTimerFromString( timerName );
    GetBEI().GetBehaviorTimerManager().GetTimer( timer  ).Reset();
  }

  OnBehaviorActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnEnteredActivatableScopeInternal()
{
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid ){
    auto& infoProcessor = GetBEI().GetAIComponent().GetAIInformationAnalyzer();
    infoProcessor.AddEnableRequest(_requiredProcess, GetDebugLabel().c_str());
  }

  // Handle Vision Mode Subscriptions
  if(!_operationModifiers.visionModesForActivatableScope->empty()){
    GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, 
      *_operationModifiers.visionModesForActivatableScope);
  }

  // Manage state for any IBEIConditions used by this Behavior
  // Conditions may be evaluted when the behavior is inside the Activatable Scope
  for(auto& strategy: _wantsToBeActivatedConditions){
    strategy->SetActive(GetBEI(), true);
  }

  OnBehaviorEnteredActivatableScope();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::OnLeftActivatableScopeInternal()
{
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid ){
    auto& infoProcessor = GetBEI().GetAIComponent().GetAIInformationAnalyzer();
    infoProcessor.RemoveEnableRequest(_requiredProcess, GetDebugLabel().c_str());
  }

  GetBEI().GetVisionScheduleMediator().ReleaseAllVisionModeSubscriptions(this);

  // Manage state for any IBEIConditions used by this Behavior
  // Conditions may not be evaluted when the behavior is outside the Activatable Scope
  for(auto& strategy: _wantsToBeActivatedConditions){
    strategy->SetActive(GetBEI(), false);
  }

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
  
  if(_hasSetIdle){
    SmartRemoveIdleAnimation();
  }

  // Set Mode Subscriptions back to ActivatableScope values. OnLeftActivatableScopeInternal handles final unsubscribe
  if(!_operationModifiers.visionModesForActivatableScope->empty()){
    GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, 
      *_operationModifiers.visionModesForActivatableScope);
  }

  // Manage state for any IBEIConditions used by this Behavior:
  // Conditions may be evaluted when inactive, if in Activatable scope
  for(auto& strategy: _wantsToBeActivatedConditions){
    strategy->SetActive(GetBEI(), true);
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
  
  if( (_respondToUserIntent != nullptr) && (!_claimUserIntentData) ) {
    // make sure a delegate of this behavior claimed the intent that we preserved for them, and
    // remove it if not
    auto& uic = GetBEI().GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
    uic.ResetPreservedUserIntents( GetID() );
  }
  
  DEV_ASSERT(_smartLockIDs.empty(), "ICozmoBehavior.Stop.DisabledReactionsNotEmpty");
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
    PRINT_NAMED_INFO("ICozmoBehavior.WantsToBeActivatedBase.DontActivateDueToCoordinator",
                    "Behavior %s was asked not to activate during tick %zu by coordinator %s", 
                    GetDebugLabel().c_str(), _tickDontActivateSetFor, _dontActivateCoordinator.c_str());
    return false;
  }

  // check if required processes are running
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid )
  {
    const bool isProcessOn = GetBEI().GetAIComponent().GetAIInformationAnalyzer().IsProcessRunning(_requiredProcess);
    if ( !isProcessOn ) {
      PRINT_NAMED_ERROR("ICozmoBehavior.WantsToBeActivated.RequiredProcessNotFound",
        "Required process '%s' is not enabled for '%s'",
        AIInformationAnalysis::StringFromEProcess(_requiredProcess),
        GetDebugLabel().c_str());
      return false;
    }
  }

  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // first check the unlock
  if ( _requiredUnlockId != UnlockId::Count )
  {
    if(GetBEI().HasProgressionUnlockComponent()){
      // ask progression component if the unlockId is currently unlocked
      auto& progressionUnlockComp = GetBEI().GetProgressionUnlockComponent();
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
    const float lastDriveOff = GetBEI().GetAIComponent().GetWhiteboard().GetTimeAtWhichRobotGotOffCharger();
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
  //check if the behavior can run from the charger platform (don't want most to run because they could damage
  //the robot by moving too much, and also will generally look dumb if they try to turn)
  if(robotInfo.IsOnChargerPlatform() && !_operationModifiers.wantsToBeActivatedWhenOnCharger){
    return false;
  }
  
  //check if the behavior can handle holding a block
  if(robotInfo.GetCarryingComponent().IsCarryingObject() && !_operationModifiers.wantsToBeActivatedWhenCarryingObject){
    return false;
  }

  for(auto& strategy: _wantsToBeActivatedConditions){
    if(!strategy->AreConditionsMet(GetBEI())){
      return false;
    }
  }
     
  return true;
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

  // Check whether we should cancel the behavior if control is no longer delegated
  if(IsActivated()){
    if(_operationModifiers.behaviorAlwaysDelegates && !IsControlDelegated()){
      if(GetBEI().HasDelegationComponent()){
        auto& delegationComponent = GetBEI().GetDelegationComponent();
        delegationComponent.CancelSelf(this);
      }
    }
  }
  
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
  Util::sEventF("robot.freeplay_objective_achieved", {{DDATA, EnumToString(objectiveAchieved)}}, "%s", GetDebugLabel().c_str());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartPushIdleAnimation(AnimationTrigger animation)
{
  if(ANKI_VERIFY(!_hasSetIdle,
                 "ICozmoBehavior.SmartPushIdleAnimation.IdleAlreadySet",
                 "Behavior %s has already set an idle animation",
                 GetDebugLabel().c_str())){
    // TODO: Restore idle animations (VIC-366)
    //robot.GetAnimationStreamer().PushIdleAnimation(animation, kIdleLockPrefix + GetDebugLabel());
    _hasSetIdle = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::SmartRemoveIdleAnimation()
{
  if(ANKI_VERIFY(_hasSetIdle,
                 "ICozmoBehavior.SmartRemoveIdleAnimation.NoIdleSet",
                 "Behavior %s is trying to remove an idle, but none is currently set",
                 GetDebugLabel().c_str())){
    // TODO: Restore idle animations (VIC-366)
    //robot.GetAnimationStreamer().RemoveIdleAnimation(kIdleLockPrefix + GetDebugLabel());
    _hasSetIdle = false;
  }
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
                                           const ObjectLights& modifier)
{
  if(std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID) == _customLightObjects.end()){
    GetBEI().GetCubeLightComponent().PlayLightAnim(objectID, anim, nullptr, true, modifier);
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
UserIntent& ICozmoBehavior::GetTriggeringUserIntent()
{
  if( ANKI_VERIFY( _pendingIntent,
                   "ICozmoBehavior.GetTriggeringUserIntent.NoIntent",
                   "Behavior '%s' called this without having been activated because of a user intent",
                   GetDebugLabel().c_str() ) )
  {
    return *_pendingIntent;
  } else {
    static UserIntent emptyIntent;
    return emptyIntent;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehavior::PlayEmergencyGetOut(AnimationTrigger anim)
{
  auto& contComp = GetBEI().GetAIComponent().GetComponent<ContinuityComponent>(AIComponentID::ContinuityComponent);
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

  
} // namespace Cozmo
} // namespace Anki
