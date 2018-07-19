/**
 * File: robotDataBackupManager.h
 *
 * Author: Al Chaussee
 * Date:   7/27/2016
 *
 * Description: Manages the robot data backups and handles picking which one to use to restore a robot
 *              Only saves backups of robot data if the robot has completed onboarding
 *              The backup file is <robot serial number>.backup
 *              The tags we are backing up are specified in backup_config.json
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_robotDataBackupManager_H__
#define __Cozmo_Basestation_robotDataBackupManager_H__

#include "coretech/common/shared/types.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/nvStorageTypes.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <unordered_map>
#include <set>


namespace Anki {

namespace Util {
  namespace Data {
    class DataPlatform;
  }
}

namespace Cozmo {
  
class Robot;
class CozmoContext;
  class NVStorageComponent;
  
class RobotDataBackupManager
{
public:
  using Tag = NVStorage::NVEntryTag;
  RobotDataBackupManager(Robot& robot);
  virtual ~RobotDataBackupManager();
  
  // Reads all tags in backup_config.json and writes the data to file
  void ReadAllBackupDataFromRobot();
  
  // Template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);

  static inline const std::string GetBackupFolder() { return "robotBackups/"; }
  
private:
  
  // ====== NVStorageComponent is a friend and should only be calling these private functions ======
  friend NVStorageComponent;
  
  // Queues the data for this tag to be written to backup
  void QueueDataToWrite(const Tag tag, const std::vector<u8> data);
  
  // Writes the queued data for this tag to file
  void WriteDataForTag(const Tag forTag, const NVStorage::NVResult res, const bool writeNotErase);
  
  // Erases all data in backup
  void WipeAll();
  // ===============================================================================================
  
  
  // Writes the data in _dataOnRobot to file as long as the robot has completed onboarding
  void WriteBackupFile();
  
  // Reads and parses the specified backup file and puts the data in dataInBackup
  using TagDataMap = std::unordered_map<u32, std::vector<u8> >;
  static bool ParseBackupFile(const std::string& fileName,
                              const std::string& pathToFile,
                              TagDataMap& dataInBackup);
  
  // Determines which backup file to use (currently selects the backup file corresponding to the robot that
  // has been connected to the most)
  static bool GetFileToUseForBackup(std::string& file,
                                    const std::string& pathToFile,
                                    Util::Data::DataPlatform* dp);
  
  // Handles increasing the connection count on robot connection
  void RobotConnected(const AnkiEvent<RobotInterface::RobotToEngine>& msg);
  
  // Whether or not this tag is one we are backing up
  bool IsValidTag(const Tag tag);
  
  bool HasPendingRequests() { return _numPendingRequests > 0; }

  Robot& _robot;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // Holds data that has been queued to be written to robot or data we are waiting on getting a write result for
  using PendingWriteDataMap = std::unordered_map<u32, std::vector<std::vector<u8> > >;
  PendingWriteDataMap _tagDataMap;
  
  // Mirrors the data stored on robot and is updated as we write to the robot
  TagDataMap _dataOnRobot;
  
  // Set of tags we are backing up (loaded from backup_config.json)
  std::set<u32> _tagsToBackup;
  
  // Path to the robotBackups folder
  const std::string kPathToFile;
  
  // Whether or not the current restore or wipe was successful
  bool _restoreOrWipeFailed = false;
  
  // Whether or not this robot has completed onboarding
  bool _hasCompletedOnboarding = false;
  
  // Whether or not we have pending NVStorage requests
  u32  _numPendingRequests = 0;
  
};
  
}
}



#endif //__Cozmo_Basestation_robotDataBackupManager_H__
