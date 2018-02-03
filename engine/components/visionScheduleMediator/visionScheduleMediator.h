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
#include "engine/dependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"

#include <set>
#include <unordered_map>
#include <vector>

namespace Anki{
namespace Cozmo{

// Forward declaration:
class VisionComponent;
class IVisionModeSubscriber;

class VisionScheduleMediator : public IDependencyManagedComponent<RobotComponentID>, public Util::noncopyable
{
public:

  VisionScheduleMediator();
  virtual ~VisionScheduleMediator();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::Vision);
    dependencies.insert(RobotComponentID::CozmoContext);
  }
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  // IDependencyManagedComponent
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Subscribe at "standard" update frequency to a set of VisionModes. This call REPLACES existing subscriptions for
  // the pertinent subscriber
  void SetVisionModeSubscriptions(IVisionModeSubscriber* subscriber, const std::set<VisionMode>& desiredModes);

  // Subscribe at defined update frequencies to a vector of VisionModes. This call REPLACES existing subscriptions for
  // the pertinent subscriber 
  void SetVisionModeSubscriptions(IVisionModeSubscriber* subscriber, const std::set<VisionModeRequest>& requests);

  // Remove all existing subscriptions for the pertinent subscriber
  void ReleaseAllVisionModeSubscriptions(IVisionModeSubscriber* subscriber);

private:

  // Internal call to parse the subscription record and send the emergent config to the VisionComponent
  void UpdateVisionSchedule();
  int GetUpdatePeriodFromEnum(const VisionMode& mode, const EVisionUpdateFrequency& frequencySetting) const;

  using RequestRecord = std::pair<IVisionModeSubscriber*, EVisionUpdateFrequency>;
  
  struct VisionModeData
  {
    uint8_t low;
    uint8_t med;
    uint8_t high;
    uint8_t standard;
    std::unordered_map<IVisionModeSubscriber*, int> requestMap;
    using record = std::pair<IVisionModeSubscriber*, int>;
    static bool CompareRecords(record i, record j) { return i.second < j.second; }
    int GetMinUpdatePeriod() const { 
      record minRecord = *min_element( requestMap.begin(), requestMap.end(), &VisionModeData::CompareRecords);
      return minRecord.second;
    }
  };

  VisionComponent* _visionComponent;
  std::unordered_map<VisionMode, VisionModeData> _modeDataMap;

}; // class VisionScheduleMediator

}// namespace Cozmo
}// namespace Anki

#endif //__Engine_Components_VisionScheduleMediator_H__
