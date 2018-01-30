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
: IDependencyManagedComponent<RobotComponentID>(RobotComponentID::VisionScheduleMediator)
{
}

VisionScheduleMediator::~VisionScheduleMediator()
{
}

void VisionScheduleMediator::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  // Get and store a ref to the vision system
  _visionComponent = &robot->GetVisionComponent();
  // Load up data from the JSON config
  auto& config = robot->GetContext()->GetDataLoader()->GetVisionScheduleMediatorConfig();

  // switch over to getting these refs from dependentComponents once that's fixed 
  // _visionComponent = &dependentComponents.at(RobotComponentID::Vision).GetValue<VisionComponent>();
  // auto& context = dependentComponents.at(RobotComponentID::CozmoContext).GetValue<CozmoContext>();
  // auto& config = context.GetDataLoader()->GetVisionScheduleMediatorConfig();

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

  return;
}

void VisionScheduleMediator::SetVisionModeSubscriptions(IVisionModeSubscriber* const subscriber,
                                                             const std::set<VisionMode>& desiredModes)
{
  // Convert to requests for "standard" update frequencies
  std::vector<VisionModeRequest> requests;
  for(auto& mode : desiredModes){
    requests.push_back({ mode, EVisionUpdateFrequency::Standard });
  }

  SetVisionModeSubscriptions(subscriber, requests);
}

void VisionScheduleMediator::SetVisionModeSubscriptions(IVisionModeSubscriber* const subscriber, 
                                                             const std::vector<VisionModeRequest>& requests)
{
  // Remove any existing subscriptions from this subscriber
  for(auto& modeDataPair : _modeDataMap){
    modeDataPair.second.requestMap.erase(subscriber);
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
    }
  }

  UpdateVisionSchedule();
}

void VisionScheduleMediator::ReleaseAllVisionModeSubscriptions(IVisionModeSubscriber* subscriber)
{
  for(auto& modeDataPair : _modeDataMap){
    modeDataPair.second.requestMap.erase(subscriber);
  }

  UpdateVisionSchedule();
}

void VisionScheduleMediator::UpdateVisionSchedule()
{
  _visionComponent->PopCurrentModeSchedule();

  // Construct a new schedule
  std::list<std::pair<VisionMode, VisionModeSchedule>> allModeScheduleList;

  for(auto& modeDataPair : _modeDataMap)
  {
    VisionMode mode = modeDataPair.first;
     const VisionModeData& modeData = modeDataPair.second;

    if(!modeData.requestMap.empty()){
      // Add the min frequency for this mode to the schedule and enable it
      int updatePeriod = modeData.GetMinUpdatePeriod();
      VisionModeSchedule schedule(updatePeriod);
      allModeScheduleList.push_back(std::pair<VisionMode, VisionModeSchedule>(mode, schedule));
      _visionComponent->EnableMode(mode, true);

      PRINT_NAMED_INFO("visionScheduleMediator.EnablingVisionMode",
        "Vision Schedule Mediator is enabling mode: %s every %d frame(s).", EnumToString(mode), updatePeriod );
    } else {
      // Set the update frequency to standard for modes with no subscribers, and disable them
      int updatePeriod = modeData.standard;
      VisionModeSchedule schedule(updatePeriod);
      allModeScheduleList.push_back(std::pair<VisionMode, VisionModeSchedule>(mode, schedule));
      _visionComponent->EnableMode(mode, false);

      PRINT_NAMED_INFO("visionScheduleMediator.DisablingVisionMode",
        "Vision Schedule Mediator is disabling mode: %s as it has no subscribers.", EnumToString(mode));
    }
  }

  AllVisionModesSchedule schedule(allModeScheduleList, false);
  _visionComponent->PushNextModeSchedule(std::move(schedule));

  // Enable modes with subscribers
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

} // namespace Cozmo
} // namespace Anki