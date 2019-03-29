/**
 * File: BehaviorSDKInterface.cpp
 *
 * Author: Michelle Sintov
 * Created: 2018-05-21
 *
 * Description: Interface for SDKs including C# and Python
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/sdkBehaviors/behaviorSDKInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sdkComponent.h"
#include "engine/components/settingsManager.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"

#define LOG_CHANNEL "BehaviorSDKInterface"

namespace Anki {
namespace Vector {

namespace {
const char* const kBehaviorControlLevelKey = "behaviorControlLevel";
const char* const kDisableCliffDetection = "disableCliffDetection";
const char* const kDriveOffChargerBehaviorKey = "driveOffChargerBehavior";
const char* const kFindAndGoToHomeBehaviorKey = "findAndGoToHomeBehavior";
const char* const kFindFacesBehaviorKey = "findFacesBehavior";
const char* const kLookAroundInPlaceBehaviorKey = "lookAroundInPlaceBehavior";
const char* const kRollBlockBehaviorKey = "rollBlockBehavior";


/* UserIntents that the SDK can relay to the user
 *
 * We don't relay messages that are consumed by higher-level behaviors
 * We don't relay messages that aren't released
 * We don't don't relay messages that are part of a multi-part behavior
 * 
 * This needs to be updated when the SDK wants to notify of new UserIntents, 
 * or if UserIntents are replaced.
 * 
 * Keep the IDs (second part of the tuples) intact to avoid breaking users.
*/
const std::set<std::tuple<UserIntentTag, unsigned int>> kIntentWhitelist = { 
  { UserIntentTag::character_age, 0 },
  { UserIntentTag::check_timer, 1 },
  { UserIntentTag::explore_start, 2 },
  { UserIntentTag::global_stop, 3 },
  { UserIntentTag::greeting_goodbye, 4 },
  { UserIntentTag::greeting_goodmorning, 5 },
  { UserIntentTag::greeting_hello, 6 },
  { UserIntentTag::imperative_abuse, 7 },
  { UserIntentTag::imperative_affirmative, 8 },
  { UserIntentTag::imperative_apology, 9 },
  { UserIntentTag::imperative_come, 10 },
  { UserIntentTag::imperative_dance, 11 },
  { UserIntentTag::imperative_fetchcube, 12 },
  { UserIntentTag::imperative_findcube, 13 },
  { UserIntentTag::imperative_lookatme, 14 },
  { UserIntentTag::imperative_love, 15 },
  { UserIntentTag::imperative_praise, 16 },
  { UserIntentTag::imperative_negative, 17 },
  { UserIntentTag::imperative_scold, 18 },
  { UserIntentTag::imperative_volumelevel, 19 },
  { UserIntentTag::imperative_volumeup, 20 },
  { UserIntentTag::imperative_volumedown, 21 },
  { UserIntentTag::movement_forward, 22 },
  { UserIntentTag::movement_backward, 23 },
  { UserIntentTag::movement_turnleft, 24 },
  { UserIntentTag::movement_turnright, 25 },
  { UserIntentTag::movement_turnaround, 26 },
  { UserIntentTag::knowledge_question, 27 },
  { UserIntentTag::names_ask, 28 },
  { UserIntentTag::play_anygame, 29 },
  { UserIntentTag::play_anytrick, 30 },
  { UserIntentTag::play_blackjack, 31 },
  { UserIntentTag::play_fistbump, 32 },
  { UserIntentTag::play_pickupcube, 33 },
  { UserIntentTag::play_popawheelie, 34 },
  { UserIntentTag::play_rollcube, 35 },
  { UserIntentTag::seasonal_happyholidays, 36 },
  { UserIntentTag::seasonal_happynewyear, 37 },
  { UserIntentTag::set_timer, 38 },
  { UserIntentTag::show_clock, 39 },
  { UserIntentTag::take_a_photo, 40 },
  { UserIntentTag::weather_response, 41 }
};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::BehaviorSDKInterface(const Json::Value& config)
 : ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig.behaviorControlLevel = JsonTools::ParseInt32(config, kBehaviorControlLevelKey, debugName);
  ANKI_VERIFY(external_interface::ControlRequest_Priority_IsValid(_iConfig.behaviorControlLevel),
              "BehaviorSDKInterface::BehaviorSDKInterface", "Invalid behaviorControlLevel %u", _iConfig.behaviorControlLevel);
  _iConfig.disableCliffDetection = JsonTools::ParseBool(config, kDisableCliffDetection, debugName);
  _iConfig.driveOffChargerBehaviorStr = JsonTools::ParseString(config, kDriveOffChargerBehaviorKey, debugName);
  _iConfig.findAndGoToHomeBehaviorStr = JsonTools::ParseString(config, kFindAndGoToHomeBehaviorKey, debugName);
  _iConfig.findFacesBehaviorStr = JsonTools::ParseString(config, kFindFacesBehaviorKey, debugName);
  _iConfig.lookAroundInPlaceBehaviorStr = JsonTools::ParseString(config, kLookAroundInPlaceBehaviorKey, debugName);
  _iConfig.rollBlockBehaviorStr = JsonTools::ParseString(config, kRollBlockBehaviorKey, debugName);

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction,
  });

  SubscribeToAppTags({
    AppToEngineTag::kDriveOffChargerRequest,
    AppToEngineTag::kDriveOnChargerRequest,
    AppToEngineTag::kFindFacesRequest,
    AppToEngineTag::kLookAroundInPlaceRequest,
    AppToEngineTag::kRollBlockRequest,
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSDKInterface::WantsToBeActivatedBehavior() const
{
  // Check whether the SDK wants control for the control level that this behavior instance is for.
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  return sdkComponent.SDKWantsControl() && (sdkComponent.SDKControlLevel()==_iConfig.behaviorControlLevel);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.findAndGoToHomeBehavior.get());
  delegates.insert(_iConfig.findFacesBehavior.get());
  delegates.insert(_iConfig.lookAroundInPlaceBehavior.get());
  delegates.insert(_iConfig.rollBlockBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.driveOffChargerBehaviorStr));
  DEV_ASSERT(_iConfig.driveOffChargerBehavior != nullptr,
             "BehaviorSDKInterface.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.findAndGoToHomeBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.findAndGoToHomeBehaviorStr));
  DEV_ASSERT(_iConfig.findAndGoToHomeBehavior != nullptr,
             "BehaviorSDKInterface.InitBehavior.NullFindAndGoToHomeBehavior");

  _iConfig.findFacesBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.findFacesBehaviorStr));
  DEV_ASSERT(_iConfig.findFacesBehavior != nullptr,
             "BehaviorSDKInterface.InitBehavior.NullFindFacesBehavior");

  _iConfig.lookAroundInPlaceBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.lookAroundInPlaceBehaviorStr));
  DEV_ASSERT(_iConfig.lookAroundInPlaceBehavior != nullptr,
             "BehaviorSDKInterface.InitBehavior.NullLookAroundInPlaceBehavior");

  _iConfig.rollBlockBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.rollBlockBehaviorStr));
  DEV_ASSERT(_iConfig.rollBlockBehavior != nullptr,
             "BehaviorSDKInterface.InitBehavior.NullRollBlockBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kBehaviorControlLevelKey,
    kDisableCliffDetection,
    kDriveOffChargerBehaviorKey,
    kFindAndGoToHomeBehaviorKey,
    kFindFacesBehaviorKey,
    kLookAroundInPlaceBehaviorKey,
    kRollBlockBehaviorKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  auto& robotInfo = GetBEI().GetRobotInfo();
  
  // Permit low level movement commands/actions to run since SDK behavior is now active.
  SetAllowExternalMovementCommands(true);

  if (_iConfig.disableCliffDetection) {
    robotInfo.EnableStopOnCliff(false);
  }

  // Tell the robot component that the SDK has been activated
  auto& sdkComponent = robotInfo.GetSDKComponent();
  sdkComponent.SDKBehaviorActivation(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::OnBehaviorDeactivated()
{
  // Tell the robot component that the SDK has been deactivated
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  sdkComponent.SDKBehaviorActivation(false);

  // Unsets eye color. Also unsets any other changes though SDK only changes eye color but this is good future proofing.
  SettingsManager& settings = GetBEI().GetSettingsManager();
  settings.ApplyAllCurrentSettings();

  // Release all track locks which may have been acquired by an SDK user
  robotInfo.GetMoveComponent().UnlockAllTracks();
  // Do not permit low level movement commands/actions to run since SDK behavior is no longer active.
  SetAllowExternalMovementCommands(false);
  // Re-enable cliff detection that SDK may have disabled
  if (_iConfig.disableCliffDetection) {
    robotInfo.EnableStopOnCliff(true);
  }
}

void BehaviorSDKInterface::ProcessUserIntents()
{
  //pull any pending user intents so we can send them to the SDK
  //then drop them so they are not used
  //only accept whitelisted events to avoid unexpecxted side-effects
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsAnyUserIntentPending() ) {
    const auto* userIntentDataPtr = uic.GetPendingUserIntent();
    if( userIntentDataPtr != nullptr ) {
      auto intent = userIntentDataPtr->intent;
      LOG_INFO("BehaviorSDKInterface::ProcessUserIntents", "Intercepted pending user untent ID %u json: %s", 
               (unsigned int)intent.GetTag(), intent.GetJSON().toStyledString().c_str());

      //Find the tuple with our tag
      auto it = std::find_if(kIntentWhitelist.begin(), kIntentWhitelist.end(), [intent](const std::tuple<UserIntentTag, unsigned int>& e) {return std::get<0>(e) == intent.GetTag();});

      if (it == kIntentWhitelist.end()) {
        //not found in list, not ours
        LOG_INFO("BehaviorSDKInterface::ProcessUserIntents", "Not forwarding user intent");
        return;
      }

      uic.DropUserIntent(intent.GetTag());

      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( gi != nullptr ) {
        auto* userIntentMsg = new external_interface::UserIntent(std::get<1>(*it), intent.GetJSON().toStyledString().c_str());
        gi->Broadcast( ExternalMessageRouter::Wrap(userIntentMsg) );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::BehaviorUpdate() 
{
  if (!IsActivated()) {
    return;
  }

  // TODO Consider which slot should be deactivated once SDK occupies multiple slots.
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  if (!sdkComponent.SDKWantsControl())
  {
    CancelSelf();
  }

  ProcessUserIntents();
}

void BehaviorSDKInterface::HandleDriveOffChargerComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResponse = new external_interface::DriveOffChargerResponse;
    driveOffChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResponse) );
  }
}  

void BehaviorSDKInterface::HandleDriveOnChargerComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResponse = new external_interface::DriveOnChargerResponse;
    driveOnChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResponse) );
  }
}

void BehaviorSDKInterface::HandleFindFacesComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* findFacesResponse = new external_interface::FindFacesResponse;
    findFacesResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(findFacesResponse) );
  }
}

void BehaviorSDKInterface::HandleLookAroundInPlaceComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* lookAroundInPlaceResponse = new external_interface::LookAroundInPlaceResponse;
    lookAroundInPlaceResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(lookAroundInPlaceResponse) );
  }
}

void BehaviorSDKInterface::HandleRollBlockComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* rollBlockResponse = new external_interface::RollBlockResponse;
    rollBlockResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(rollBlockResponse) );
  }
}

// Reports back to gateway that requested actions have been completed.
// E.g., the Python SDK ran play_animation and wants to know when the animation
// action was completed.
void BehaviorSDKInterface::HandleWhileActivated(const EngineToGameEvent& event)
{
  if (IsControlDelegated()) {
    // The SDK behavior has delegated to another behavior, and that
    // behavior requested an action. Don't inform gateway that the
    // action has completed because it wasn't requested by the SDK.
    //
    // If necessary, can delegate to actions from the behavior instead
    // of running them via CLAD request from gateway.
    return;
  }

  if (event.GetData().GetTag() != EngineToGameTag::RobotCompletedAction) {
    return;
  }

  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();

  ExternalInterface::RobotCompletedAction msg = event.GetData().Get_RobotCompletedAction();
  sdkComponent.OnActionCompleted(msg);
}

void BehaviorSDKInterface::SetAllowExternalMovementCommands(const bool allow) {
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetMoveComponent().AllowExternalMovementCommands(allow, GetDebugLabel());
}

void BehaviorSDKInterface::HandleWhileActivated(const AppToEngineEvent& event) {
  LOG_INFO("BehaviorSDKInterface::HandleWhileActivated", "Received AppToEngine message %u", (unsigned int)event.GetData().GetTag());
  switch(event.GetData().GetTag())
  {
    case external_interface::GatewayWrapperTag::kDriveOffChargerRequest:
      DriveOffChargerRequest(event.GetData().drive_off_charger_request());
      break;

    case external_interface::GatewayWrapperTag::kDriveOnChargerRequest:
      DriveOnChargerRequest(event.GetData().drive_on_charger_request());
      break;

    case external_interface::GatewayWrapperTag::kFindFacesRequest:
      FindFacesRequest(event.GetData().find_faces_request());
      break;

    case external_interface::GatewayWrapperTag::kLookAroundInPlaceRequest:
      LookAroundInPlaceRequest(event.GetData().look_around_in_place_request());
      break;

    case external_interface::GatewayWrapperTag::kRollBlockRequest:
      RollBlockRequest(event.GetData().roll_block_request());
      break;

    default:
    {
      PRINT_NAMED_WARNING("BehaviorSDKInterface.HandleWhileActivated.NoMatch", "No match for action tag so no response sent: [Tag=%d]", (int)event.GetData().GetTag());
      return;
    }
  }
}

// Delegate to the DriveOffCharger behavior
void BehaviorSDKInterface::DriveOffChargerRequest(const external_interface::DriveOffChargerRequest& driveOffChargerRequest) {
  if (_iConfig.driveOffChargerBehavior->WantsToBeActivated()) {
    LOG_INFO("BehaviorSDKInterface::DriveOffChargerRequest", "Delegating to DriveOffCharger behavior");
    if (DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(), &BehaviorSDKInterface::HandleDriveOffChargerComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  LOG_ERROR("BehaviorSDKInterface::DriveOffChargerRequest", "Behavior did not want to be activated/delegated");
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResponse = new external_interface::DriveOffChargerResponse;
    driveOffChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResponse) );
  }
}

// Delegate to FindAndGoToHome
void BehaviorSDKInterface::DriveOnChargerRequest(const external_interface::DriveOnChargerRequest& driveOnChargerRequest) {
  if (_iConfig.findAndGoToHomeBehavior->WantsToBeActivated()) {
    LOG_INFO("BehaviorSDKInterface::DriveOnChargerRequest", "Delegating to DriveOnCharger behavior");
    if (DelegateIfInControl(_iConfig.findAndGoToHomeBehavior.get(), &BehaviorSDKInterface::HandleDriveOnChargerComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  LOG_ERROR("BehaviorSDKInterface::DriveOnChargerRequest", "Behavior did not want to be activated/delegated");
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResponse = new external_interface::DriveOnChargerResponse;
    driveOnChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResponse) );
  }
}

// Delegate to the FindFaces behavior
void BehaviorSDKInterface::FindFacesRequest(const external_interface::FindFacesRequest& findFacesRequest) {
  if (_iConfig.findFacesBehavior->WantsToBeActivated()) {
    LOG_INFO("BehaviorSDKInterface::FindFacesRequest", "Delegating to FindFaces behavior");

    if (DelegateIfInControl(_iConfig.findFacesBehavior.get(), &BehaviorSDKInterface::HandleFindFacesComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  LOG_ERROR("BehaviorSDKInterface::FindFacesRequest", "Behavior did not want to be activated/delegated");
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* findFacesResponse = new external_interface::FindFacesResponse;
    findFacesResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(findFacesResponse) );
  }
}

// Delegate to the LookAroundInPlace behavior
void BehaviorSDKInterface::LookAroundInPlaceRequest(const external_interface::LookAroundInPlaceRequest& lookAroundInPlaceRequest) {
  if (_iConfig.lookAroundInPlaceBehavior->WantsToBeActivated()) {
    LOG_INFO("BehaviorSDKInterface::LookAroundInPlaceRequest", "Delegating to LookAroundInPlace behavior");
    if (DelegateIfInControl(_iConfig.lookAroundInPlaceBehavior.get(), &BehaviorSDKInterface::HandleLookAroundInPlaceComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  LOG_ERROR("BehaviorSDKInterface::LookAroundInPlaceRequest", "Behavior did not want to be activated/delegated");
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* lookAroundInPlaceResponse = new external_interface::LookAroundInPlaceResponse;
    lookAroundInPlaceResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(lookAroundInPlaceResponse) );
  }
}

// Delegate to the RollBlock behavior
void BehaviorSDKInterface::RollBlockRequest(const external_interface::RollBlockRequest& rollBlockRequest) {
  if (_iConfig.rollBlockBehavior->WantsToBeActivated()) {
    LOG_ERROR("BehaviorSDKInterface::RollBlockRequest", "Delegating to RollBlock behavior");
    if (DelegateIfInControl(_iConfig.rollBlockBehavior.get(), &BehaviorSDKInterface::HandleRollBlockComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  LOG_ERROR("BehaviorSDKInterface::RollBlockRequest", "Behavior did not want to be activated/delegated");
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* rollBlockResponse = new external_interface::RollBlockResponse;
    rollBlockResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(rollBlockResponse) );
  }
}

} // namespace Vector
} // namespace Anki
