/**
 * File: bleRobotManager.h
 *
 * Description: C++ class wrapper around Objective-C BLEVehicleManager functions
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_BLE_ROBOT_MANAGER_H_
#define BASESTATION_BLE_ROBOT_MANAGER_H_

#include "anki/common/types.h"
#include <vector>

namespace Anki {
namespace Cozmo {


class BLERobotManager {
public:

  // Default constructor
  BLERobotManager();

  // The destructor will automatically cleans up
  ~BLERobotManager();
  
  void DiscoverRobots();
  void StopDiscoveringRobots();

  void Update();

  // Returns robotID given mfgID.
  // Returns -1 if mfgID not among the visible robots.
  int GetRobotIDFromMfgID(u64 mfgID);
  
  // Populates mfgIDs with all advertising robot manufacturing IDs
  int GetAdvertisingRobotMfgIDs(std::vector<u64> &mfgIDs);
  
  // Returns the number of connected robots.
  u32 GetNumConnectedRobots();

  // Connects to the robot with the given manufacturing ID
  void ConnectToRobotByMfgID(u64 mfgID);
  
};

}  // namespace Cozmo
}  // namespace Anki
#endif  // #ifndef BASESTATION_BLE_ROBOT_MANAGER_H_

