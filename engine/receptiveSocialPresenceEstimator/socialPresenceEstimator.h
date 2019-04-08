/**
 * File: socialPresenceEstimator.h
 *
 * Author: Andrew Stout
 * Created: 2019-04-02
 *
 * Description: Estimates whether someone receptive to social engagement is available.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __VICTOR_SOCIALPRESENCEESTIMATOR_H__
#define __VICTOR_SOCIALPRESENCEESTIMATOR_H__

#include "engine/robot.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

// forward declarations
class Robot;


class SocialPresenceEstimator : public IDependencyManagedComponent<RobotComponentID>,
                                private Util::noncopyable
{
public:
  explicit SocialPresenceEstimator();
  ~SocialPresenceEstimator();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  //virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {}
  //virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {}
  //virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& components) const override {}

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

private:
  // -------------------------- Private Member Funcs ---------------------------
  void SubscribeToWebViz();
  std::vector<Signal::SmartHandle> _signalHandles;

  // -------------------------- Private Member Vars ----------------------------
  Robot* _robot = nullptr;
  //float _lastWebVizSendTime_s = 0.0f;
};

}
}




#endif // __VICTOR_SOCIALPRESENCEESTIMATOR_H__
