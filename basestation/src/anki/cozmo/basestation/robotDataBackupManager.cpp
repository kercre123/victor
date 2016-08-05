/**
 * File: robotDataBackupManager.cpp
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

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataBackupManager.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/onboardingData.h"
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {
  
RobotDataBackupManager::RobotDataBackupManager(Robot& robot)
: _robot(robot)
, kPathToFile((_robot.GetContextDataPlatform() != nullptr ? _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, "robotBackups/") : ""))
{
  if(_robot.HasExternalInterface())
  {
    _signalHandles.push_back(_robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(),
                                                                        RobotInterface::RobotToEngineTag::robotAvailable,
                                                                        std::bind(&RobotDataBackupManager::RobotConnected, this, std::placeholders::_1)));
    
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RestoreRobotFromBackup>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::WipeRobotGameData>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestRobotRestoreData>();
  }
  
  if(Util::FileUtils::CreateDirectory(kPathToFile, false, true) == false)
  {
    PRINT_NAMED_ERROR("RobotDataBackupManager.RestoreFromBackup.CreateDir", "Unable to create directory for robot backups");
    return;
  }
  
  const std::string config = (_robot.GetContextDataPlatform() != nullptr ? _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Resources, kBackupConfig) : "");
  if(Util::FileUtils::FileExists(config))
  {
    Json::Value json;
    if(!_robot.GetContextDataPlatform()->readAsJson(config, json))
    {
      PRINT_NAMED_ERROR("RobotDataBackupManager.Constructor.ReadJson", "Failed to read backup_config.json");
      return;
    }
    
    for(auto tag : json["tagsToBackup"].getMemberNames())
    {
      _tagsToBackup.insert(json["tagsToBackup"][tag].asUInt());
    }
  }
}

RobotDataBackupManager::~RobotDataBackupManager()
{
  WriteBackupFile();
}

void RobotDataBackupManager::WriteBackupFile()
{
  // Only save a backup if the robot has compeleted onboarding
  if(!_hasCompletedOnboarding)
  {
    PRINT_NAMED_INFO("RobotDataBackupManager.WriteBackupFile", "Not completed onboarding not writing backup");
    return;
  }
  
  PRINT_NAMED_INFO("RobotDataBackupManager.WriteBackupFile", "Writing backup file");
  
  // Writes the data in _dataOnRobot to file
  std::vector<uint8_t> contents;
  for(auto iter = _dataOnRobot.begin(); iter != _dataOnRobot.end(); ++iter)
  {
    size_t size = iter->second.Size();
    uint8_t blob[size];
    iter->second.Pack(blob, size);
    contents.insert(contents.end(), blob, blob + size);
  }
  
  if(_fileName == "")
  {
    PRINT_NAMED_ERROR("RobotDataBackupManager.Destructor", "Filename not set");
    return;
  }
  
  Util::FileUtils::WriteFile(kPathToFile + _fileName, contents);

}

void RobotDataBackupManager::RobotConnected(const AnkiEvent<RobotInterface::RobotToEngine>& msg)
{
  const RobotInterface::RobotAvailable& payload = msg.GetData().Get_robotAvailable();
  const std::string serialNumber = std::to_string(payload.robotID);
  Json::Value stats;
  
  // Reads the json file statsForBackup and increments the connection count for this robot
  if(Util::FileUtils::FileExists(kPathToFile + kStatsForBackupFile))
  {
    if(!_robot.GetContextDataPlatform()->readAsJson(kPathToFile + kStatsForBackupFile, stats))
    {
      PRINT_NAMED_ERROR("RobotDataBackupManager.RobotConnected.ReadStatsFailed", "Failed to read %s", kStatsForBackupFile.c_str());
      return;
    }
    else
    {
      stats[kConnectionCountKey][serialNumber] = stats[kConnectionCountKey][serialNumber].asUInt() + 1;
    }
  }
  else
  {
    stats[kConnectionCountKey][serialNumber] = 1;
  }
  
  if(!_robot.GetContextDataPlatform()->writeAsJson(Util::Data::Scope::Persistent, "robotBackups/" + kStatsForBackupFile, stats))
  {
    PRINT_NAMED_ERROR("RobotDataBackupManager.RobotConnected.WriteStatsFailed", "Failed to write backup stats file");
  }
  
  _fileName = serialNumber + ".backup";
}

bool RobotDataBackupManager::IsValidTag(const Tag tag)
{
  return _tagsToBackup.find(static_cast<u32>(tag)) != _tagsToBackup.end();
}
  
void RobotDataBackupManager::ReadAllBackupDataFromRobot()
{
  if(HasPendingRequests())
  {
    PRINT_NAMED_WARNING("RobotDataBackupManager.ReadAllBackupDataFromRobot",
                        "Have pending requests so not reading");
    return;
  }

  // Reads all the tags that we want to backup
  int count = 0;
  const size_t tagsToBackupSize = _tagsToBackup.size();
  _numPendingRequests = 0;
  for(auto iter = _tagsToBackup.begin(); iter != _tagsToBackup.end(); ++iter)
  {
    ++count;
    const u32 tag = *iter;
    ++_numPendingRequests;
    _robot.GetNVStorageComponent().Read(static_cast<Tag>(tag),
                                        [this, tag, count, tagsToBackupSize](u8* data, size_t size, NVStorage::NVResult res)
                                        {
                                          --_numPendingRequests;
                                          if(res != NVStorage::NVResult::NV_OKAY)
                                          {
                                            PRINT_NAMED_WARNING("RobotDataBackupManager.ReadAllDataFromRobot",
                                                                "Reading tag %s failed",
                                                                EnumToString(static_cast<Tag>(tag)));
                                          }
                                          else
                                          {
                                            NVStorage::NVStorageBlob blob;
                                            blob.blob = std::vector<u8>(data, data + size);
                                            blob.tag = tag;
                                            
                                            _dataOnRobot[tag] = blob;
                                            
                                            // If this is the onboarding tag then set _hasCompletedOnboarding
                                            // This used to determine if we should be writing backup files for this
                                            // robot
                                            if(tag == static_cast<u32>(NVStorage::NVEntryTag::NVEntry_OnboardingData))
                                            {
                                              OnboardingData onboardingData;
                                              onboardingData.Unpack(data, size);
                                              _hasCompletedOnboarding = onboardingData.hasCompletedOnboarding;
                                            }
                                          }
                                          
                                          // Once we have read all the tags write the backup file
                                          if(count == tagsToBackupSize)
                                          {
                                            WriteBackupFile();
                                          }
                                        });
  }
}

void RobotDataBackupManager::QueueDataToWrite(const Tag tag, const std::vector<u8> data)
{
  if(!IsValidTag(tag))
  {
    PRINT_NAMED_DEBUG("RobotDataBackupManager.QueueDataToWrite", "Ignoring invalid tag %s[%d]",
                      EnumToString(tag), static_cast<u32>(tag));
    return;
  }
  
  NVStorage::NVStorageBlob blob;
  blob.blob = data;
  blob.tag = static_cast<u32>(tag);
  _tagDataMap[static_cast<u32>(tag)].push_back(blob);
}

void RobotDataBackupManager::WriteDataForTag(const Tag forTag, const NVStorage::NVResult res)
{
  u32 tag = static_cast<u32>(forTag);
  auto iter = _tagDataMap.find(tag);
  
  // We need to have called QueueDataToWrite with a given tag before calling this function
  if(iter == _tagDataMap.end())
  {
    PRINT_NAMED_INFO("RobotDataBackupManager.WriteDataForTag",
                     "Tag %s[%d] doesn't exist in tagDataMap",
                     EnumToString(forTag), tag);
    return;
  }
  
  // If the write to robot failed then the data for this tag wasn't actually written so erase it
  if(res != NVStorage::NVResult::NV_OKAY)
  {
    _tagDataMap.erase(iter);
    return;
  }
  
  _dataOnRobot[tag] = _tagDataMap[tag].front();
  if(_tagDataMap[tag].erase(_tagDataMap[tag].begin()) == _tagDataMap[tag].end())
  {
    _tagDataMap.erase(iter);
  }
  
  // Special case for if we are writting to the onboarding tag then we need to update _hasCompletedOnboarding
  if(forTag == NVStorage::NVEntryTag::NVEntry_OnboardingData)
  {
    OnboardingData data;
    data.Unpack(_dataOnRobot[tag].blob.data(), _dataOnRobot[tag].blob.size());
    _hasCompletedOnboarding = data.hasCompletedOnboarding;
  }
  
  WriteBackupFile();
}

// Restores the connected robot using the data in a backup file
// If robotToRestoreFrom is 0 (default) then the file will be automatically selected using
// GetFileToUseForBackup otherwise the file corresponding to the id in robotToRestoreFrom will be used
template<>
void RobotDataBackupManager::HandleMessage(const ExternalInterface::RestoreRobotFromBackup& msg)
{
  if(HasPendingRequests())
  {
    PRINT_NAMED_WARNING("RobotDataBackupManager.RestoreRobotFromBackup",
                        "Have pending requests so not restoring");
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(false, false)));
    return;
  }

  if(false == Util::FileUtils::CreateDirectory(kPathToFile, false, true))
  {
    PRINT_NAMED_INFO("RobotDataBackupManager.RestoreFromBackup.NoDir", "Failed to create backup dir");
    return;
  }
  
  std::string fileName;
  
  // If robotToRestoreFrom is zero (we want to automatically pick a file to restore from)
  // or the file we want to restore from doesn't exist then try to automatically pick a file to restore with
  if(msg.robotToRestoreFrom == 0 ||
     !Util::FileUtils::FileExists(kPathToFile + std::to_string(msg.robotToRestoreFrom) + ".backup"))
  {
    if(!GetFileToUseForBackup(fileName))
    {
      PRINT_NAMED_ERROR("RobotDataBackupManager.RestoreFromBackup.NoBackup", "No backup to restore from");
      return;
    }
  }
  // Otherwise we are going to use the restore file that is from the specified robot
  else
  {
    fileName = std::to_string(msg.robotToRestoreFrom) + ".backup";
  }
  
  PRINT_NAMED_INFO("RobotDataBackupManager.RestoreFromBackup", "Using file %s for backup", fileName.c_str());
  
  std::string file = Util::FileUtils::ReadFile(kPathToFile + fileName);
  
  if(file.length() == 0)
  {
    PRINT_NAMED_ERROR("RobotDataBackupManager.RestoreFromBackup.ReadFileFailed", "Unable to read backup file");
    return;
  }
  const char* fileContents = file.data();
  
  // Clear both of these since they are no longer valid as we are restoring
  _dataOnRobot.clear();
  _tagDataMap.clear();
  
  // Since the only way we could have a restore file is if the corresponding robot had completed onboarding
  // then this robot will technically have completed onboarding too due to the restore
  _hasCompletedOnboarding = true;
  
  // Parses the contents of the backup file into blobs and adds them to _dataOnRobot
  bool finishedParsing = false;
  uint32_t sectionStart = 0;
  while(!finishedParsing)
  {
    uint32_t tag = (fileContents[sectionStart] & 0xff) |
    (fileContents[sectionStart+1] & 0xff) << 8 |
    (fileContents[sectionStart+2] & 0xff) << 16 |
    (fileContents[sectionStart+3] & 0xff) << 24;
    
    sectionStart += 4;
    
    uint32_t blob_length = (fileContents[sectionStart] & 0xff) |
    (fileContents[sectionStart+1] & 0xff) << 8 |
    (fileContents[sectionStart+2] & 0xff) << 16 |
    (fileContents[sectionStart+3] & 0xff) << 24;
    
    sectionStart += 4;
    
    NVStorage::NVStorageBlob b;
    b.tag = tag;
    std::vector<u8> temp(fileContents+sectionStart, fileContents+sectionStart+blob_length);
    b.blob.swap(temp);
    
    sectionStart += blob_length;
    
    _dataOnRobot[b.tag] = b;
    
    if(sectionStart >= file.length())
    {
      finishedParsing = true;
    }
  }

  _restoreOrWipeFailed = false;

  // Write all the data in the restore file to robot and write a new backup file on completion
  int count = 0;
  _numPendingRequests = 0;
  const size_t dataOnRobotSize = _dataOnRobot.size();
  for(auto iter : _dataOnRobot)
  {
    NVStorage::NVEntryTag tag = static_cast<NVStorage::NVEntryTag>(iter.first);
    const u8* data = iter.second.blob.data();
    ++count;
    ++_numPendingRequests;
    _robot.GetNVStorageComponent().Write(tag, data, iter.second.blob.size(),
                                         [this, tag, count, dataOnRobotSize](NVStorage::NVResult res)
                                         {
                                           --_numPendingRequests;
                                           if(res != NVStorage::NVResult::NV_OKAY)
                                           {
                                             PRINT_NAMED_WARNING("RobotDataBackupManager.RestoreFromBackup",
                                                                 "Writing to tag %s failed with %s",
                                                                 EnumToString(tag),
                                                                 EnumToString(res));
                                             _restoreOrWipeFailed = true;
                                             // Let game know the restore failed
                                             _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(false, false)));
                                           }
                                           else
                                           {
                                             if(count == dataOnRobotSize && !_restoreOrWipeFailed)
                                             {
                                               PRINT_NAMED_INFO("RobotDataBackupManager.RestoreFromBackup",
                                                                "Restore complete!");
                                               // Let game know that the restore was successful
                                               _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(false, true)));
                                               WriteBackupFile();
                                             }
                                        
                                           }
                                         });
  }
}

bool RobotDataBackupManager::GetFileToUseForBackup(std::string& file)
{
  // Check to see if we have a backup for the robot
  if(Util::FileUtils::FileExists(kPathToFile + _fileName))
  {
    PRINT_NAMED_INFO("RobotDataBackupManager.GetFileToUseForBackup",
                     "Have exisiting backup for current robot using it to restore");
    file = _fileName;
    return true;
  }

  // We don't have an exisiting backup so read the connection counts in statsForBackup.json
  // and find the robot that we have connected the most to
  Json::Value stats;
  if(Util::FileUtils::FileExists(kPathToFile + kStatsForBackupFile))
  {
    if(!_robot.GetContextDataPlatform()->readAsJson(kPathToFile + kStatsForBackupFile, stats))
    {
      PRINT_NAMED_ERROR("RobotDataBackupManager.GetFileToUseForBackup.ReadJson.Failed",
                        "Failed to read %s",
                        kStatsForBackupFile.c_str());
      return false;
    }
    else
    {
      u32 highestConnectionCount = 0;
      for(auto member : stats[kConnectionCountKey].getMemberNames())
      {
        u32 count = stats[kConnectionCountKey][member].asUInt();
        if( count > highestConnectionCount)
        {
          highestConnectionCount = count;
          file = member;
        }
      }
      
      file += ".backup";
      return true;
    }
  }
  else
  {
    PRINT_NAMED_ERROR("RobotDataBackupManager.GetFileToUseForBackup.ReadJson.NoFile", "No backup stats file found");
    return false;
  }
}

// Wipes all tags specified in backup_config.json
template<>
void RobotDataBackupManager::HandleMessage(const ExternalInterface::WipeRobotGameData& msg)
{
  if(HasPendingRequests())
  {
    PRINT_NAMED_WARNING("RobotDataBackupManager.WipeRobotGameData",
                        "Have pending requests so not wiping");
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(false, false)));
    return;
  }

  PRINT_NAMED_INFO("RobotDataBackupManager.WipeRobotGameData", "Wiping robot game data");
  _tagDataMap.clear();
  _dataOnRobot.clear();
  _hasCompletedOnboarding = false;
  
  // Erase all of the tags we care about
  int count = 0;
  _restoreOrWipeFailed = false;
  _numPendingRequests = 0;
  const size_t tagsToBackupSize = _tagsToBackup.size();
  for(const auto& tag : _tagsToBackup)
  {
    ++count;
    ++_numPendingRequests;
    _robot.GetNVStorageComponent().Erase(static_cast<Tag>(tag),
                                         [this, tag, count, tagsToBackupSize](NVStorage::NVResult res)
                                         {
                                           --_numPendingRequests;
                                           if(res != NVStorage::NVResult::NV_OKAY)
                                           {
                                             PRINT_NAMED_WARNING("RobotDataBackupManager.WipeRobotGameData",
                                                                 "Erase of tag %s failed with %s",
                                                                 EnumToString(static_cast<Tag>(tag)),
                                                                 EnumToString(res));
                                             _restoreOrWipeFailed = true;
                                             // Let game know that the wipe failed
                                             _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(true, false)));
                                           }
                                           else if(count == tagsToBackupSize && !_restoreOrWipeFailed)
                                           {
                                             PRINT_NAMED_INFO("RobotDataBackupManager.WipeRobotGameData",
                                                              "Wipe Complete!");
                                             // Let game know that the wipe was a success
                                             _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(true, true)));
                                             // Don't need to update the backup file here since it won't
                                             // write the file as this robot has not completed onboarding after
                                             // being wiped
                                           }
                                         });
  }
}

// Handles sending game information about the backups we have and whether or not we should prompt the user
// to restore the current robot
template<>
void RobotDataBackupManager::HandleMessage(const ExternalInterface::RequestRobotRestoreData& msg)
{
  ExternalInterface::RestoreRobotOptions restoreOptions;
  restoreOptions.shouldPromptForRestore = false;
  
  // Finds the names of all the backup files and reports them to game along with whether or not we should
  // prompt the user to restore this robot
  std::vector<std::string> files = Util::FileUtils::FilesInDirectory(kPathToFile);
  
  for(std::string file : files)
  {
    // Remove file ending to just get name of file which is a robot serial number
    auto start = file.find(".backup");
    if(start != std::string::npos)
    {
      file.erase(start);
    }
    else
    {
      continue;
    }
    
    PRINT_NAMED_INFO("RobotDataBackupManager.RobotConnected.AllBackups", "Have backup for %llu", std::stoull(file));
    restoreOptions.robotsWithBackupData.push_back((u32)std::stoull(file));
    
    // We should only prompt for restore if there exists a backup and this robot has not completed onboarding
    restoreOptions.shouldPromptForRestore = !_hasCompletedOnboarding;
  }
  
  PRINT_NAMED_INFO("RobotDataBackupManager.HandleRequestRobotRestoreData",
                   "Will prompt for restore %s",
                   (restoreOptions.shouldPromptForRestore ? "True" : "False"));
  
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(restoreOptions)));
}
  
}
}