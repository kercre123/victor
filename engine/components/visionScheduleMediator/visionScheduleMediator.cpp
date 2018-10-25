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
#include "engine/vision/visionModeSet.h"
#include "engine/vision/visionModesHelpers.h"
#include "webServerProcess/src/webService.h"

namespace Anki{
namespace Vector{

// If UpdatePeriods longer than this become necessary, INCREASE IT. This will also increase
// the number of frames of the schedule displayed in Webots which will require an appropriate
// increase in the width of "victor_vision_mode_display" in CozmoVisDisplay.proto to display 
// correctly.
const uint8_t kMaxUpdatePeriod = 16;

// Sanity checking relative cost values. This can be increased if necessary to accomodate
// higher cost VisionModes.
const uint8_t kMaxRelCost = 50;

const std::string kWebVizModuleName = "visionschedulemediator";

VisionScheduleMediator::VisionScheduleMediator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::VisionScheduleMediator)
{
}

VisionScheduleMediator::~VisionScheduleMediator()
{
}

void VisionScheduleMediator::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  // Load up data from the JSON config
  auto& config = 
    dependentComps.GetComponent<ContextWrapper>().context->GetDataLoader()->GetVisionScheduleMediatorConfig();

  Init(config);
}

void VisionScheduleMediator::Init(const Json::Value& config)
{
  using namespace JsonTools;
  const char* debugName = "VisionScheduleMediator";
  const Json::Value& settingsArray = config["VisionModeSettings"];
  for(const auto& modeSettings : settingsArray)
  {
    std::string modeName = ParseString(modeSettings, "mode", debugName);
    VisionMode visionMode = VisionModeFromString(modeName);

    VisionModeData newModeData = { .low = ParseUint8(modeSettings, "low", debugName),
                                   .med = ParseUint8(modeSettings, "med", debugName),
                                   .high = ParseUint8(modeSettings, "high", debugName),
                                   .standard = ParseUint8(modeSettings, "standard", debugName),
                                   .relativeCost = ParseUint8(modeSettings, "relativeCost", debugName)};

    // Range check incoming params
    DEV_ASSERT((newModeData.low > 0) && (newModeData.low <= kMaxUpdatePeriod) && 
               (newModeData.med > 0) && (newModeData.med <= kMaxUpdatePeriod) &&
               (newModeData.high > 0) && (newModeData.high <= kMaxUpdatePeriod) &&
               (newModeData.standard > 0) && (newModeData.standard <= kMaxUpdatePeriod) &&
               (newModeData.relativeCost > 0) && (newModeData.relativeCost <= kMaxRelCost),
               "VisionScheduleMediator.VisionModeFrequencyOutOfRange");

    // Limit inputs to Power-Of-Two (zero check is covered above)
    // To simplify schedule building, VisionModeFrequencies must be POT 
    DEV_ASSERT(((newModeData.low & (newModeData.low  - 1)) == 0) &&
               ((newModeData.med & (newModeData.med  - 1)) == 0) &&
               ((newModeData.high & (newModeData.high  - 1)) == 0) &&
               ((newModeData.standard & (newModeData.standard  - 1)) == 0),
               "VisionScheduleMediator.NonPOTVisionModeFrequency");

    _modeDataMap.insert(std::pair<VisionMode, VisionModeData>(visionMode, newModeData));

    _schedule = AllVisionModesSchedule({}, true);
  }

  // Set up baseline subscriptions to manage VisionMode defaults via the VSM
  std::set<VisionModeRequest> baselineSubscriptions;
  GetInternalSubscriptions(baselineSubscriptions);
  SetVisionModeSubscriptions(this, baselineSubscriptions);
}

void VisionScheduleMediator::UpdateDependent(const RobotCompMap& dependentComps)
{
  // Update the VisionSchedule, if necessary
  if(_subscriptionRecordIsDirty){
    UpdateVisionSchedule(dependentComps.GetComponent<VisionComponent>(),
                         dependentComps.GetComponent<ContextWrapper>().context);
  }

  // Update the visualization tools
  if(ANKI_DEV_CHEATS){
    // Send every 50 frames to update corner cases for the vizManager e.g. after init
    if(_framesSinceSendingDebugViz++ >= 50){
      SendDebugVizMessages(dependentComps.GetComponent<ContextWrapper>().context);
      _framesSinceSendingDebugViz = 0;
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
    if(modeDataPair.second.requestMap.erase(subscriber) > 0){
      modeDataPair.second.dirty = true;
      _subscriptionRecordIsDirty = true;
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
      _subscriptionRecordIsDirty = true;
    }
  }
}

  
void VisionScheduleMediator::DevOnly_SelfSubscribeVisionMode(const VisionModeSet& modes)
{
  for(const VisionMode& mode : modes) {
    auto got = _modeDataMap.find(mode);
    if(got != _modeDataMap.end()) {
      got->second.requestMap.emplace(this, 1);
      got->second.dirty = true;
      _subscriptionRecordIsDirty = true;
    }
  }

}
  
void VisionScheduleMediator::DevOnly_SelfUnsubscribeVisionMode(const VisionModeSet& modes)
{
  for(const VisionMode& mode : modes) {
    auto got = _modeDataMap.find(mode);
    if(got != _modeDataMap.end()) {
      got->second.requestMap.erase(this);
      got->second.dirty = true;
      _subscriptionRecordIsDirty = true;
    }
  }
}

void VisionScheduleMediator::ReleaseAllVisionModeSubscriptions(IVisionModeSubscriber* subscriber)
{
  // Prevent subscription releases using nullptr
  DEV_ASSERT(nullptr != subscriber, "VisionScheduleMediator.NullVisionModeSubscriber");
  if(nullptr == subscriber){
    return;
  }

  for(auto& modeDataPair : _modeDataMap){
    if(modeDataPair.second.requestMap.erase(subscriber) > 0){
      modeDataPair.second.dirty = true;
      _subscriptionRecordIsDirty = true;
    }
  }
}

void VisionScheduleMediator::UpdateVisionSchedule(VisionComponent& visionComponent, const CozmoContext* context)
{
  // Construct a new schedule
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
        modeEnabledChanged = true;
        activeModesDirty = true;
      }

      // Compute the update period for active modes and add to the schedule
      if(UpdateModePeriodIfNecessary(modeData)){
        modeScheduleChanged = true;
        scheduleDirty = true;
      }
      
      if(modeData.enabled) {
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
    auto modeScheduleList = GenerateBalancedSchedule(visionComponent);
    const bool kUseDefaultsForUnspecified = true;
    _schedule = AllVisionModesSchedule(modeScheduleList, kUseDefaultsForUnspecified);
  }

  // On any occasion where we made updates, update the debug viz
  if(ANKI_DEV_CHEATS && (scheduleDirty || activeModesDirty)){
    SendDebugVizMessages(context);
  }

  _subscriptionRecordIsDirty = false;
}

const AllVisionModesSchedule::ModeScheduleList VisionScheduleMediator::GenerateBalancedSchedule(
  VisionComponent& visionComponent)
{
  std::vector<uint8_t> costStackup(kMaxUpdatePeriod);
  uint8_t maxRequestedUpdatePeriod = 0;

  AllVisionModesSchedule::ModeScheduleList modeScheduleList;

  for(auto& modeData : _modeDataMap){
    if(!modeData.second.enabled){
      continue;
    }

    const VisionMode mode = modeData.first;
    const uint8_t updatePeriod = modeData.second.updatePeriod;
    const uint8_t relativeCost = modeData.second.relativeCost;

    // Keep track of our longest requested UpdatePeriod to search the full schedule minimally
    if (updatePeriod > maxRequestedUpdatePeriod){
      maxRequestedUpdatePeriod = updatePeriod;
    }

    uint8_t minCostOffset = 0;
    if(updatePeriod > 1){
      if(updatePeriod == maxRequestedUpdatePeriod){
        // This element will occur only once in the Min Repeating Pattern find the min cost element and put it there
        auto endSearchIterator = costStackup.begin() + maxRequestedUpdatePeriod;
        minCostOffset = std::distance(costStackup.begin(), min_element(costStackup.begin(), endSearchIterator));
      } else {
        // This element will repeat within the Min Repeating Pattern. Use MiniMax type algorithm
        int minCost = std::numeric_limits<int>::max();
        for(int offset = 0; offset < updatePeriod; ++offset){
          int totalCost = 0;
          for(int index = offset; index < maxRequestedUpdatePeriod; index += updatePeriod){
            totalCost += costStackup[index];
          }
          if(totalCost < minCost){
            minCost = totalCost;
            minCostOffset = offset;
          }
        }
      }
    }

    // Reframe minCostOffset in the set [0, updatePeriod)
    minCostOffset %= updatePeriod;

    // Having found the desired offset, populate the costStackup record
    modeData.second.offset = minCostOffset;
    for(int i = minCostOffset; i < kMaxUpdatePeriod; i += updatePeriod){
      costStackup[i] += relativeCost;
    }

    DEV_ASSERT((minCostOffset < updatePeriod), "VisionScheduleMediator.FrameOffsetOutOfBounds");
    VisionModeSchedule schedule(updatePeriod, minCostOffset);
    modeScheduleList.push_back({ mode, schedule });
  }

  return modeScheduleList;
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

void VisionScheduleMediator::SendDebugVizMessages(const CozmoContext* context)
{
  VizInterface::VisionModeDebug data;
  Json::Value webVizData;
  uint8_t numActiveModes = 0;
  webVizData["patternWidth"] = kMaxUpdatePeriod;
  Json::Value& fullSchedule = webVizData["fullSchedule"]; 

  // Debug display strings for schedules of all vision modes
  for(VisionMode whichMode = VisionMode(0); whichMode < VisionMode::Count; ++whichMode)
  {
    char schedule[kMaxUpdatePeriod + 1] = {0};
      
    for(int j = 0; j < kMaxUpdatePeriod; j++){
      schedule[j] = (_schedule.GetScheduleForMode(whichMode).IsTimeToProcess(j) ? '1' : '0');
    }

    std::string modeString(schedule);
    modeString += " ";
    modeString += EnumToString(whichMode);
    data.debugStrings.push_back(modeString);
  }

  
  for(const auto& modeDataPair : _modeDataMap) {
    if(modeDataPair.second.enabled || modeDataPair.second.updatePeriod != 0){
      numActiveModes++;

      // Store data for WebViz
      Json::Value modeSchedule;
      modeSchedule["visionMode"] = EnumToString(modeDataPair.first);
      modeSchedule["updatePeriod"] = modeDataPair.second.updatePeriod;
      modeSchedule["offset"] = modeDataPair.second.offset;
      fullSchedule.append(modeSchedule);
    }
  }

  if(nullptr != context){
    // Send data to Webots
    VizManager* vizManager = context->GetVizManager();
    if(nullptr != vizManager){
      vizManager->SendVisionModeDebug(std::move(data));
    }

    // Send data to WebViz
    const auto* webService = context->GetWebService();
    if( webService != nullptr ){
      webVizData["numActiveModes"] = numActiveModes; 
      webService->SendToWebViz( kWebVizModuleName, webVizData );
    }
  }
}

} // namespace Vector
} // namespace Anki
