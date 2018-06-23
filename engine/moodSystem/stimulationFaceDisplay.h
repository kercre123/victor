/**
 * File: stimulationFaceDisplay.h
 *
 * Author: Brad Neuman
 * Created: 2018-03-28
 *
 * Description: Component which uses the "Stimulated" mood to modify the robot's face (current design is to
 *              modify saturation level)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_MoodSystem_StimulationFaceDisplay_H__
#define __Engine_MoodSystem_StimulationFaceDisplay_H__

#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

namespace Util {
class GraphEvaluator2d;
}

namespace Cozmo {

class StimulationFaceDisplay : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:

  StimulationFaceDisplay();
  ~StimulationFaceDisplay();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {
    components.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::MoodManager);
  }
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& components) const override {
    components.insert(RobotComponentID::Animation);
  }

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

private:

  float _lastSentSatLevel = -1.0f;

  // for tracking the exponential moving average
  float _ema = 0.0f;

  // mapping to turn ema value into display value
  std::unique_ptr<Util::GraphEvaluator2d> _saturationMap;
  
};

}
}


#endif
