/**
 * File: inventoryComponent.h
 *
 * Author: Molly Jameson
 * Created: 2017-05-24
 *
 * Description: A component to manage inventory
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_InventoryComponent_H__
#define __Anki_Cozmo_Basestation_Components_InventoryComponent_H__

#include "clad/types/inventoryTypes.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <chrono>
#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;
class CozmoContext;

class InventoryComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  static const int kInfinity = -1;

  explicit InventoryComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContext);
  }
  virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {
    components.insert(RobotComponentID::CozmoContext);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void SetInventoryAmount(InventoryType inventoryID, int total);
  void AddInventoryAmount(InventoryType inventoryID, int delta);
  int  GetInventoryAmount(InventoryType inventoryID) const;
  int  GetInventorySpaceRemaining(InventoryType inventoryID) const;
  
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  void SendInventoryAllToGame();

private:
  Robot* _robot = nullptr;

  void ReadInventoryConfig(const Json::Value& config);
  void TryWriteCurrentInventoryToRobot();
  void WriteCurrentInventoryToRobot();
  void ReadCurrentInventoryFromRobot();
  
  int GetInventoryCap(InventoryType inventoryID) const;
  
  // clad array for easy unpacking
  InventoryList _currentInventory;
  bool          _readFromRobot;
  
  std::chrono::time_point<std::chrono::system_clock>  _timeLastWrittenToRobot;
  bool          _robotWritePending;
  bool          _isWritingToRobot;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  std::map<InventoryType, int>  _inventoryTypeCaps;
};

}
}

#endif
