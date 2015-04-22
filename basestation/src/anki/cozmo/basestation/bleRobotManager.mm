/**
 * File: bleVehicleManagerCPP.cpp
 *
 * Author: Kevin Yoon
 * Created: 10/13/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "bleRobotManager.h"
#include "BLEManager/BLEVehicleManager.h"


namespace Anki {
namespace Cozmo {

  BLERobotManager::BLERobotManager()
  {
    [BLEVehicleManager sharedInstance];
    
    // This loop seems to be necessary for CBCentralManager to power up!
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01f]];
  }
  
  // The destructor will automatically cleans up
  BLERobotManager::~BLERobotManager() {}
  
  void BLERobotManager::DiscoverRobots()
  {
    printf("Calling DiscoverRobots\n");
    [[BLEVehicleManager sharedInstance] discoverVehicles];
  }
  
  void BLERobotManager::StopDiscoveringRobots()
  {
    [[BLEVehicleManager sharedInstance] stopDiscoveringVehicles];
  }
  

  int BLERobotManager::GetAdvertisingRobotMfgIDs(std::vector<u64> &mfgIDs)
  {
    mfgIDs.clear();
    
    BLEVehicleManager *vehicleMgr = [BLEVehicleManager sharedInstance];
    for (BLEVehicleConnection* vehicleConn in vehicleMgr.vehicleConnections) {
      mfgIDs.push_back(vehicleConn.mfgID);
    }
    
    return mfgIDs.size();
  }
  
  void BLERobotManager::ConnectToRobotByMfgID(u64 mfgID)
  {
    [[BLEVehicleManager sharedInstance] connectToVehicleByMfgID:mfgID];
  }
  
  int BLERobotManager::GetRobotIDFromMfgID(u64 mfgID)
  {
    BLEVehicleManager *vehicleMgr = [BLEVehicleManager sharedInstance];
    for (BLEVehicleConnection* vehicleConn in vehicleMgr.vehicleConnections) {
      if (vehicleConn.mfgID == mfgID) {
        return vehicleConn.vehicleID;
      }
    }
    
    return -1;
  }
  
  void BLERobotManager::Update() {
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01f]];
  }
  
  u32 BLERobotManager::GetNumConnectedRobots()
  {
    u32 connectedCount = 0;
    
    BLEVehicleManager *vehicleMgr = [BLEVehicleManager sharedInstance];
    for (BLEVehicleConnection* vehicleConn in vehicleMgr.vehicleConnections) {
      if (vehicleConn.connectionState == kConnectedPipeReady) {
        ++connectedCount;
      }
    }
    
    return connectedCount;
  }
  

}  // namespace Cozmo
}  // namespace Anki


