/**
 * File: visionScheduleMediator
 *
 * Author: Sam Russell
 * Date:   1/16/2018
 *
 * Description: Mediator class providing an abstracted API through which outside
 *              systems can request vision mode scheduling. The mediator tracks
 *              all such requests and constructs a unified vision mode schedule
 *              to service them in a more optimal way
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "visionScheduleMediator.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

namespace Anki{
namespace Cozmo{

VisionScheduleMediator::VisionScheduleMediator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::VisionScheduleMediator)
{
}

VisionScheduleMediator::~VisionScheduleMediator()
{
}

void VisionScheduleMediator::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  // Get and store a ref to the vision system
  _visionComponent = &robot->GetVisionComponent();
  // Get and store a ref to the Context
  _context = robot->GetContext();
  // Load up data from the JSON config
  auto& config = _context->GetDataLoader()->GetVisionScheduleMediatorConfig();

  using namespace JsonTools;
  const char* debugName = "VisionScheduleMediator";
  const Json::Value& settingsArray = config["VisionModeSettings"];
  for(const auto& modeSettings : settingsArray)
  {
    std::string modeName = ParseString(modeSettings, "mode", debugName);
    VisionMode visionMode = VisionModeFromString(modeName);
    DEV_ASSERT(visionMode != VisionMode::Idle, "VisionScheduleMediator.InvalidVisionModeInJsonConfig");

    VisionModeData newModeData = { .low = ParseUint8(modeSettings, "low", debugName),
                                       .med = ParseUint8(modeSettings, "med", debugName),
                                       .high = ParseUint8(modeSettings, "high", debugName),
                                       .standard = ParseUint8(modeSettings, "standard", debugName)};

    _modeDataMap.insert(std::pair<VisionMode, VisionModeData>(visionMode, newModeData));
  }

  // Set up baseline subscriptions to manage VisionMode defaults via the VSM
  std::set<VisionModeRequest> baselineSubscriptions;
  GetInternalSubscriptions(baselineSubscriptions);
  SetVisionModeSubscriptions(this, baselineSubscriptions);

  return;
}

void VisionScheduleMediator::Update()
{
  // Update the VisionSchedule, if necessary
  if(subscriptionRecordIsDirty){
    UpdateVisionSchedule();
  }

  // Update the visualization tools
  if(ANKI_DEV_CHEATS){
    // Send every 50 frames to update corner cases for the vizManager e.g. after init
    if(framesSinceSendingDebugViz++ >= 50){
      SendDebugVizMessages();
      framesSinceSendingDebugViz = 0;
    }
  } 
}

void VisionScheduleMediator::SetVisionModeSubscriptions(IVisionModeSubscriber* const subscriber,
                                                        const std::set<VisionMode>& desiredModes)
{
  // Convert to requests for "standard" update frequencies
  std::set<VisionModeRequest> requests;
  for(auto& mode : desiredModes){
    requests.insert({ mode, EVisionUpdateFrequency::Standard });
  }

  SetVisionModeSubscriptions(subscriber, requests);
}

void VisionScheduleMediator::SetVisionModeSubscriptions(IVisionModeSubscriber* const subscriber,
                                                        const std::set<VisionModeRequest>& requests)
{
  // Prevent subscriptions using nullptr
  DEV_ASSERT(nullptr != subscriber, "VisionScheduleMediator.NullVisionModeSubscriber");
  if(nullptr == subscriber){
    return;
  }

  // Remove any existing subscriptions from this subscriber
  for(auto& modeDataPair : _modeDataMap){
    if(modeDataPair.second.requestMap.erase(subscriber)){
      modeDataPair.second.dirty = true;
    }
  }

  for(auto& request : requests)
  {
    auto modeDataIterator = _modeDataMap.find(request.mode);
    if(modeDataIterator == _modeDataMap.end()){
      PRINT_NAMED_ERROR("VisionScheduleMediator.UnknownVisionMode",
        "Vision mode %s was requested by a subscriber, missing settings in visionScheduleMediator_config.json",
        EnumToString(request.mode));
    } else {
      // Record the new request
      int updatePeriod_images = GetUpdatePeriodFromEnum(request.mode, request.frequency);
      VisionModeData& modeData = modeDataIterator->second;
      modeData.requestMap.emplace( subscriber, updatePeriod_images );
      modeData.dirty = true;
    }
  }

  subscriptionRecordIsDirty = true;
}

void VisionScheduleMediator::ReleaseAllVisionModeSubscriptions(IVisionModeSubscriber* subscriber)
{
  // Prevent subscription releases using nullptr
  DEV_ASSERT(nullptr != subscriber, "VisionScheduleMediator.NullVisionModeSubscriber");
  if(nullptr == subscriber){
    return;
  }

  for(auto& modeDataPair : _modeDataMap){
    if(modeDataPair.second.requestMap.erase(subscriber)){
      modeDataPair.second.dirty = true;
    }
  }

  subscriptionRecordIsDirty = true;
}

void VisionScheduleMediator::UpdateVisionSchedule()
{
  // Construct a new schedule
  std::list<std::pair<VisionMode, VisionModeSchedule>> allModeScheduleList;
  bool scheduleDirty = false;
  bool activeModesDirty = false;

  for(auto& modeDataPair : _modeDataMap)
  {
    auto& mode = modeDataPair.first;
    auto& modeData = modeDataPair.second;
    bool modeEnabledChanged = false;
    bool modeScheduleChanged = false;

    if(modeData.dirty){
      // Check if the mode should be enabled or disabled
      if(modeData.requestMap.empty() == modeData.enabled){
        modeData.enabled = !modeData.requestMap.empty();
        _visionComponent->EnableMode(mode, modeData.enabled);
        modeEnabledChanged = true;
        activeModesDirty = true;
      }

      // Compute the update period for active modes and add to the schedule
      if(modeData.enabled) {
        if(UpdateModePeriodIfNecessary(modeData)){
          VisionModeSchedule schedule(modeData.updatePeriod);
          allModeScheduleList.push_back({mode, schedule});
          modeScheduleChanged = true;
          scheduleDirty = true;
        }
        if(modeEnabledChanged || modeScheduleChanged){
          PRINT_NAMED_INFO("visionScheduleMediator.EnablingVisionMode",
                           "Vision Schedule Mediator is enabling mode: %s every %d frame(s).", 
                           EnumToString(mode),
                           modeData.updatePeriod );
        } else{
          PRINT_NAMED_INFO("visionScheduleMediator.StateUnchanged",
                           "Subscription changes for mode: %s did not result in changes to the VisionMode schedule",
                           EnumToString(mode));
        }

      } else {
        // Any dirty mode which is now disabled should have no subscribers
        DEV_ASSERT(modeData.requestMap.empty(), "visionScheduleMediator.ModeHasBrokenSubscribers");
        PRINT_NAMED_INFO("visionScheduleMediator.DisablingVisionMode",
                         "Vision Schedule Mediator is disabling mode: %s as it has no subscribers.",
                         EnumToString(mode));
      }

      modeData.dirty = false;
    }
  }

  if(scheduleDirty){
    AllVisionModesSchedule schedule(allModeScheduleList, false);
    _visionComponent->PopCurrentModeSchedule();
    _visionComponent->PushNextModeSchedule(std::move(schedule));
  }

  // On any occasion where we made updates, update the debug viz
  if(ANKI_DEV_CHEATS && (scheduleDirty || activeModesDirty)){
    SendDebugVizMessages();
  }

  subscriptionRecordIsDirty = false;
}

bool VisionScheduleMediator::UpdateModePeriodIfNecessary(VisionModeData& modeData) const
{
  int updatePeriod = modeData.GetMinUpdatePeriod();
  if(updatePeriod != modeData.updatePeriod){
    modeData.updatePeriod = updatePeriod;
    return true;
  }

  return false;
}

int VisionScheduleMediator::GetUpdatePeriodFromEnum(const VisionMode& mode, 
                                                    const EVisionUpdateFrequency& frequencySetting) const
{
  const VisionModeData& modeData = _modeDataMap.at(mode);
  switch(frequencySetting)
  {
    case EVisionUpdateFrequency::High:
      return modeData.high;
      break;
    case EVisionUpdateFrequency::Med:
      return modeData.med;
      break;
    case EVisionUpdateFrequency::Low:
      return modeData.low;
      break;
    case EVisionUpdateFrequency::Standard:
      return modeData.standard;
      break;
    default:
      DEV_ASSERT(false, "VisionScheduleMediator.UnknownFrequencyEnum");
      return 0;
      break;
  }
}

void VisionScheduleMediator::SendDebugVizMessages()
{
  VizInterface::VisionModeDebug data;

  for(auto& modeDataPair : _modeDataMap) {
    if(modeDataPair.second.enabled){
      std::string modeString(EnumToString(modeDataPair.first));
      data.debugStrings.push_back(modeString);
    }
  }

  VizManager* vizManager = _context->GetVizManager();
  if(nullptr != vizManager){
    vizManager->SendVisionModeDebug(std::move(data));
  }
}

} // namespace Cozmo
} // namespace Anki
