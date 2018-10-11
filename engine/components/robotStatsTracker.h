/**
 * File: robotStatsTracker.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-07
 *
 * Description: Persistent storage of robot lifetime stats
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_RobotStatsTracker_H__
#define __Engine_Components_RobotStatsTracker_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent_fwd.h"
#include "engine/robotComponents_fwd.h"
#include "osState/wallTime.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "json/json.h"


namespace Anki {
namespace Vector {

enum class ActiveFeature : uint32_t;
enum class BehaviorStat : uint32_t;

class JdocsManager;
  
class RobotStatsTracker : public IDependencyManagedComponent<RobotComponentID>,
                          public UnreliableComponent<BCComponentID>, 
                          private Anki::Util::noncopyable
{
public:

  RobotStatsTracker();
  ~RobotStatsTracker();

  void IncreaseStimulationSeconds(float delta);
  void IncreaseStimulationCumulativePositiveDelta(float delta);
  
  void IncrementActiveFeature(const ActiveFeature& feature, const std::string& intentSource);

  // specifically for petting duration
  void IncrementPettingDuration(const float secondsPet);

  void IncrementBehaviorStat(const BehaviorStat& stat);

  void IncrementNamedFacesPerDay();

  void IncreaseOdometer(float lWheelDelta_mm, float rWheelDelta_mm, float bodyDelta_mm);

  float GetNumHoursAlive() const;

  ////////////////////////////////////////////////////////////////////////////////

  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::JdocsManager);
  }
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  // prevent hiding warnings (use the robot component versions for real)
  using UnreliableComponent<BCComponentID>::InitDependent;
  using UnreliableComponent<BCComponentID>::GetInitDependencies;
  using UnreliableComponent<BCComponentID>::UpdateDependent;
  
  static Json::Value FilterStatsForApp(const Json::Value& json);

private:

  // reset everything including file backup. This is destructive, be careful!
  void ResetAllStats();
  
  void IncreaseHelper(const std::string& prefix, const std::string& stat, uint64_t delta);

  bool UpdateStatsJdoc(const bool saveToDiskImmediately,
                       const bool saveToCloudImmediately = false);

  void DoJdocFormatMigration();

  bool _dirtyJdoc = false;
  JdocsManager* _jdocsManager = nullptr;
  float _timeOfNextAliveTimeCheck = 0.0f;
};

}
}

#endif
