/**
* File: appCubeConnectionSubscriber.h
*
* Author: Matt Michini
* Created: 7/11/18
*
* Description: Class which handles cube connection requests from the app/sdk layer
*              and subscribes to the CubeConnectionCoordinator as necessary.
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Components_AppCubeConnectionSubscriber_H__
#define __Engine_Components_AppCubeConnectionSubscriber_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h" // some useful typedefs
#include "engine/components/cubes/iCubeConnectionSubscriber.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki{
namespace Vector{

class IGatewayInterface;

class AppCubeConnectionSubscriber : public IDependencyManagedComponent<RobotComponentID>, public ICubeConnectionSubscriber
{
public:
  AppCubeConnectionSubscriber();
  ~AppCubeConnectionSubscriber();

  // Dependency Managed Component Methods
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override;
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override {};
  // end Dependency Managed Component Methods

  // Cube Connection Subscriber Methods
  virtual std::string GetCubeConnectionDebugName() const override;
  virtual void ConnectedCallback(CubeConnectionType connectionType) override;
  virtual void ConnectionFailedCallback() override;
  virtual void ConnectionLostCallback() override;
  // end Cube Connection Subscriber Methods

private:

  void HandleAppRequest(const AppToEngineEvent& event);

  Robot* _robot;
  IGatewayInterface* _gi = nullptr;
  std::set<AppToEngineTag> _appToEngineTags;
  std::vector<Signal::SmartHandle> _eventHandles;
};


}
}

#endif //__Engine_Components_AppCubeConnectionSubscriber_H__
