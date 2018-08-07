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

#ifndef __Engine_Components_VisionScheduleMediator_H__
#define __Engine_Components_VisionScheduleMediator_H__

#include "visionScheduleMediator_fwd.h"

#include "clad/types/visionModes.h"
#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "engine/robotComponents_fwd.h"
#include "engine/vision/visionModeSchedule.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include <set>
#include <unordered_map>
#include <vector>

namespace Anki{
namespace Vector{

// Forward declaration:
class CozmoContext;
class VisionComponent;
class IVisionModeSubscriber;

class VisionScheduleMediator : public IDependencyManagedComponent<RobotComponentID>,
                               public Util::noncopyable,
                               public IVisionModeSubscriber
{
public:

  VisionScheduleMediator();
  virtual ~VisionScheduleMediator();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::Vision);
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  }
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {}
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::Vision);
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  // IDependencyManagedComponent
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Perform most of the Init from here, called by InitDependent, to allow for Unit Testing
  void Init(const Json::Value& config);

  // Set up baseline subscriptions to manage VisionMode defaults via the VSM
  void GetInternalSubscriptions(std::set<VisionModeRequest>& baselineSubscriptions) const {
    baselineSubscriptions.insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
  }

  // Subscribe at "standard" update frequency to a set of VisionModes. This call REPLACES existing subscriptions for
  // the pertinent subscriber
  void SetVisionModeSubscriptions(IVisionModeSubscriber* subscriber, const std::set<VisionMode>& desiredModes);

  // Subscribe at defined update frequencies to a vector of VisionModes. This call REPLACES existing subscriptions for
  // the pertinent subscriber 
  void SetVisionModeSubscriptions(IVisionModeSubscriber* subscriber, const std::set<VisionModeRequest>& requests);

  // Remove all existing subscriptions for the pertinent subscriber
  void ReleaseAllVisionModeSubscriptions(IVisionModeSubscriber* subscriber);

  // in debug builds, send viz messages to webots
  void SendDebugVizMessages(const CozmoContext* context);
  
  // for webots testing purposes:
  // VSM will subscribe itself to the passed in modes, which will allow
  // vision modes to be enabled from non-engine processes (i.e. from
  // webotsCtrlBuildServerTest)
  void DevOnly_SelfSubscribeVisionMode(const std::set<VisionMode>& modes);
  void DevOnly_SelfUnsubscribeVisionMode(const std::set<VisionMode>& modes);

private:

  struct VisionModeData
  {
    uint8_t low;
    uint8_t med;
    uint8_t high;
    uint8_t standard;
    uint8_t relativeCost = 1;
    bool    enabled = false;
    bool    dirty = false;
    uint8_t updatePeriod = 0;
    uint8_t offset = 0;
    std::unordered_map<IVisionModeSubscriber*, int> requestMap;
    using record = std::pair<IVisionModeSubscriber*, int>;
    static bool CompareRecords(record i, record j) { return i.second < j.second; }
    int GetMinUpdatePeriod() const { 
      record minRecord = *min_element( requestMap.begin(), requestMap.end(), &VisionModeData::CompareRecords);
      return minRecord.second;
    }
  };

  // Internal call to parse the subscription record and send the emergent config to the VisionComponent if it changed
  void UpdateVisionSchedule(VisionComponent& visionComponent, const CozmoContext* context);

  // Makes a pass over the VisionModeSchedule to spread out processing requirements for various modes and sends it to 
  // the VisionComponent. The generated schedule is returned for evaluation in Unit Tests, but is not normally used.
  const AllVisionModesSchedule::ModeScheduleList GenerateBalancedSchedule(VisionComponent& visionComponent);

  // Returns true if the update period for this mode changed as a result of subscription changes
  bool UpdateModePeriodIfNecessary(VisionModeData& mode) const;

  // Helper method to convert between enums and settings in (frames between updates)
  int GetUpdatePeriodFromEnum(const VisionMode& mode, const EVisionUpdateFrequency& frequencySetting) const;

  using RequestRecord = std::pair<IVisionModeSubscriber*, EVisionUpdateFrequency>;

  std::unordered_map<VisionMode, VisionModeData> _modeDataMap;
  bool _subscriptionRecordIsDirty = false;
  bool _hasScheduleOnStack = false;
  uint8_t _framesSinceSendingDebugViz = 0;

}; // class VisionScheduleMediator

}// namespace Vector
}// namespace Anki

#endif //__Engine_Components_VisionScheduleMediator_H__
