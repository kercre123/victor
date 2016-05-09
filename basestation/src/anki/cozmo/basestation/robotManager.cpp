//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "anki/cozmo/basestation/events/gameEventResponsesContainer.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/firmwareUpdater/firmwareUpdater.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/gameEvent.h"
#include "util/fileUtils/fileUtils.h"
#include "util/time/stepTimers.h"
#include <sys/stat.h>

#include "util/global/globalDefinitions.h"
#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"
#endif

namespace Anki {
  namespace Cozmo {
    
    RobotManager::RobotManager(const CozmoContext* context)
    : _context(context)
    , _robotEventHandler(context)
    , _cannedAnimations(new CannedAnimationContainer())
    , _animationGroups(new AnimationGroupContainer())
    , _firmwareUpdater(new FirmwareUpdater(context))
    , _gameEventResponses(new GameEventResponsesContainer())
    {
      
    }
    
    RobotManager::~RobotManager() = default;
    
    void RobotManager::Init()
    {
      auto startTime = std::chrono::system_clock::now();
    
      Anki::Util::Time::PushTimedStep("RobotManager::Init");
      
      Anki::Util::Time::PushTimedStep("ReadAnimationDir");
      ReadAnimationDir();
      Anki::Util::Time::PopTimedStep();
      
      Anki::Util::Time::PushTimedStep("ReadAnimationGroupDir");
      ReadAnimationGroupDir();
      Anki::Util::Time::PopTimedStep();
      
      Anki::Util::Time::PushTimedStep("ReadAnimationGroupMapsDir");
      _gameEventResponses->Load(_context->GetDataPlatform(),"assets/AnimationGroupMaps");
      Anki::Util::Time::PopTimedStep();
      
      Anki::Util::Time::PopTimedStep(); // RobotManager::Init
      
      Anki::Util::Time::PrintTimedSteps();
      Anki::Util::Time::ClearSteps();
      
      auto endTime = std::chrono::system_clock::now();
      auto timeSpent_millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
      
      if (ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS)
      {
        constexpr auto maxInitTime_millis = 3000;
        if (timeSpent_millis > maxInitTime_millis)
        {
          PRINT_NAMED_WARNING("RobotManager.Init.TimeSpent",
                              "%lld milliseconds spent initializing, expected %d",
                              timeSpent_millis,
                              maxInitTime_millis);
        }
      }
      
      PRINT_NAMED_EVENT("RobotManager.Init.TimeSpent", "%lld milliseconds", timeSpent_millis);
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      if (_robots.find(withID) == _robots.end()) {
        PRINT_STREAM_INFO("RobotManager.AddRobot", "Adding robot with ID=" << withID);
        _robots[withID] = new Robot(withID, _context);
        _IDs.push_back(withID);
      } else {
        PRINT_STREAM_WARNING("RobotManager.AddRobot.AlreadyAdded", "Robot with ID " << withID << " already exists. Ignoring.");
      }
    }
    
    
    void RobotManager::RemoveRobot(const RobotID_t withID)
    {
      auto iter = _robots.find(withID);
      if(iter != _robots.end()) {
        PRINT_NAMED_INFO("RobotManager.RemoveRobot", "Removing robot with ID=%d\n", withID);
        
        _robotDisconnectedSignal.emit(withID);
        _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDisconnected(withID, 0.0f)));
        
        
        delete(iter->second);
        iter = _robots.erase(iter);
        
        // Find the ID. This is inefficient, but this isn't a long list
        for(auto idIter = _IDs.begin(); idIter != _IDs.end(); ++idIter) {
          if(*idIter == withID) {
            _IDs.erase(idIter);
            break;
          }
        }
      } else {
        PRINT_NAMED_WARNING("RobotManager.RemoveRobot", "Robot %d does not exist. Ignoring.\n", withID);
      }
    }
    
    std::vector<RobotID_t> const& RobotManager::GetRobotIDList() const
    {
      return _IDs;
    }

    // for when you don't care and you just want a damn robot
    Robot* RobotManager::GetFirstRobot()
    {
      if (_IDs.empty()) {
        return nullptr;
      }
      return GetRobotByID(_IDs.front());
    }

    // Get a pointer to a robot by ID
    Robot* RobotManager::GetRobotByID(const RobotID_t robotID)
    {
      auto it = _robots.find(robotID);
      if (it != _robots.end()) {
        return it->second;
      }
      
      PRINT_NAMED_WARNING("RobotManager.GetRobotByID.InvalidID", "No robot with ID=%d", robotID);
      
      return nullptr;
    }
    
    size_t RobotManager::GetNumRobots() const
    {
      return _robots.size();
    }
    
    bool RobotManager::DoesRobotExist(const RobotID_t withID) const
    {
      return _robots.count(withID) > 0;
    }

    
    bool RobotManager::InitUpdateFirmware(int version)
    {
      return _firmwareUpdater->InitUpdate(_robots, version);
    }
    
    
    bool RobotManager::UpdateFirmware()
    {
      return _firmwareUpdater->Update(_robots);
    }
    
    
    void RobotManager::UpdateAllRobots()
    {
      //for (auto &r : _robots) {
      for(auto r = _robots.begin(); r != _robots.end(); ) {
        // Call update
        const RobotID_t robotId = r->first; // have to cache this prior to any ++r calls...
        Robot* robot = r->second;
        Result result = robot->Update();
        
        switch(result)
        {
          case RESULT_FAIL_IO_TIMEOUT:
          {
            PRINT_NAMED_WARNING("RobotManager.UpdateAllRobots.FailIOTimeout", "Signaling robot disconnect\n");
            const RobotID_t robotIdToRemove = r->first;
            ++r;
            RemoveRobot(robotIdToRemove);
            
            break;
          }
            
            // TODO: Handle other return results here
            
          default:
            // No problems, simply move to next robot
            ++r;
            break;
        }

        if(robot->HasReceivedRobotState()) {
          _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(robot->GetRobotState()));
        } else {
          PRINT_NAMED_WARNING("RobotManager.UpdateAllRobots",
                              "Not sending robot %d state (none available).",
                              robotId);
        }
      } // End loop on _robots
      
    }
    
    void RobotManager::ReadAnimationDir()
    {
      if (nullptr == _context || nullptr ==_context->GetDataPlatform())
      {
        ASSERT_NAMED("RobotManager.ReadAnimations","No context or data platform for reading animations!");
      }
      
      // Disable super-verbose warnings about clipping face parameters in json files
      // To help find bad/deprecated animations, try removing this.
      ProceduralFace::EnableClippingWarning(false);
      
      ReadAnimationDirImpl("assets/animations/");
      ReadAnimationDirImpl("config/basestation/animations/");
      
      ProceduralFace::EnableClippingWarning(true);
      
      BroadcastAvailableAnimations();
    }
    
    void RobotManager::ReadAnimationDirImpl(const std::string& animationDir)
    {
      Anki::Util::Time::ScopedStep scopeTimer(animationDir.c_str());
      
      const std::string animationFolder =
      _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, animationDir);
      
      auto filePaths = Util::FileUtils::FilesInDirectory(animationFolder, true, "json", true);
      
      for (auto path : filePaths)
      {
        struct stat attrib{0};
        int result = stat(path.c_str(), &attrib);
        if (result == -1) {
          PRINT_NAMED_WARNING("RobotManager.ReadAnimationFile", "could not get mtime for %s", path.c_str());
          continue;
        }
        bool loadFile = false;
        auto mapIt = _loadedAnimationFiles.find(path);
#ifdef __APPLE__  // TODO: COZMO-1057
        time_t tmpSeconds = attrib.st_mtimespec.tv_sec;
#else
        time_t tmpSeconds = attrib.st_mtime;
#endif
        if (mapIt == _loadedAnimationFiles.end()) {
          _loadedAnimationFiles.insert({path, tmpSeconds});
          loadFile = true;
        } else {
          if (mapIt->second < tmpSeconds) {
            mapIt->second = tmpSeconds;
            loadFile = true;
          } else {
            //PRINT_NAMED_INFO("Robot.ReadAnimationFile", "old time stamp for %s", fullFileName.c_str());
          }
        }
        if (loadFile) {
          ReadAnimationFile(path.c_str());
        }
      }
#if ANKI_DEV_CHEATS && !ANDROID
      // Only when not shipping use our temp dir
      std::string test_anim = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, USBTunnelServer::TempAnimFileName);
      if( Util::FileUtils::FileExists(test_anim) )
      {
        ReadAnimationFile(test_anim.c_str());
      }
#endif
    }
    
    void RobotManager::BroadcastAvailableAnimations()
    {
      Anki::Util::Time::ScopedStep scopeTimer("BroadcastAvailableAnimations");
      // Tell UI about available animations
      if (nullptr != _context->GetExternalInterface()) {
        std::vector<std::string> animNames(_cannedAnimations->GetAnimationNames());
        for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
          _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationAvailable>(*i);
        }
      }
    }
    
    // Read the animation data from a file
    void RobotManager::ReadAnimationFile(const char* filename)
    {
      Json::Value animDefs;
      const bool success = _context->GetDataPlatform()->readAsJson(filename, animDefs);
      std::string animationId;
      if (success && !animDefs.empty()) {
        //PRINT_NAMED_DEBUG("Robot.ReadAnimationFile", "reading %s", filename);
        _cannedAnimations->DefineFromJson(animDefs, animationId);
        
        if(std::string(filename).find(animationId) == std::string::npos) {
          PRINT_NAMED_WARNING("RobotManager.ReadAnimationFile.AnimationNameMismatch",
                              "Animation name '%s' does not match seem to match "
                              "filename '%s'", animationId.c_str(), filename);
        }
      }
    }
    
    // Read the animationGroups in a dir
    void RobotManager::ReadAnimationGroupDir()
    {
      if (nullptr == _context || nullptr ==_context->GetDataPlatform())
      {
        ASSERT_NAMED("RobotManager.ReadAnimationGroupDir","No context or data platform for reading animation groups!");
      }
      
      const std::string animationGroupFolder =
      _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "assets/animationGroups/");
      
      auto filePaths = Util::FileUtils::FilesInDirectory(animationGroupFolder, true, "json",true);
      for (auto path : filePaths)
      {
        struct stat attrib{0};
        int result = stat(path.c_str(), &attrib);
        if (result == -1) {
          PRINT_NAMED_WARNING("RobotManager.ReadAnimationGroupFile", "could not get mtime for %s", path.c_str());
          continue;
        }
        bool loadFile = false;
        auto mapIt = _loadedAnimationGroupFiles.find(path);
#ifdef __APPLE__  // TODO: COZMO-1057
        time_t tmpSeconds = attrib.st_mtimespec.tv_sec;
#else
        time_t tmpSeconds = attrib.st_mtime;
#endif
        if (mapIt == _loadedAnimationGroupFiles.end()) {
          _loadedAnimationGroupFiles.insert({path, tmpSeconds});
          loadFile = true;
        } else {
          if (mapIt->second < tmpSeconds) {
            mapIt->second = tmpSeconds;
            loadFile = true;
          } else {
            //PRINT_NAMED_INFO("Robot.ReadAnimationGroupFile", "old time stamp for %s", fullFileName.c_str());
          }
        }
        if (loadFile) {
          ReadAnimationGroupFile(path.c_str());
        }
      }
      
      // TODO: Implement external interface
      /*
       // Tell UI about available animationGroups
       if (nullptr != _context->GetExternalInterface()) {
       std::vector<std::string> animNames(_cannedAnimationGroups.GetAnimationGroupNames());
       for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
       _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::AnimationGroupAvailable(*i)));
       }
       }
       */
    }
    
    // Read the animation groups in a dir
    void RobotManager::ReadAnimationGroupFile(const char* filename)
    {
      Json::Value animGroupDef;
      const bool success = _context->GetDataPlatform()->readAsJson(filename, animGroupDef);
      if (success && !animGroupDef.empty()) {
        
        std::string fullName(filename);
        
        // remove path
        auto slashIndex = fullName.find_last_of("/");
        std::string jsonName = slashIndex == std::string::npos ? fullName : fullName.substr(slashIndex + 1);
        // remove extension
        auto dotIndex = jsonName.find_last_of(".");
        std::string animationGroupName = dotIndex == std::string::npos ? jsonName : jsonName.substr(0, dotIndex);
        
        PRINT_NAMED_INFO("RobotManager.ReadAnimationGroupFile", "reading %s - %s", animationGroupName.c_str(), filename);
        
        _animationGroups->DefineFromJson(animGroupDef, animationGroupName);
      }
    }
    
    bool RobotManager::HasCannedAnimation(const std::string& animName)
    {
      return _cannedAnimations->HasAnimation(animName);
    }
    bool RobotManager::HasAnimationGroup(const std::string& groupName)
    {
      return _animationGroups->HasGroup(groupName);
    }
    bool RobotManager::HasAnimationResponseForEvent( GameEvent ev )
    {
      return _gameEventResponses->HasResponse(ev);
    }
    std::string RobotManager::GetAnimationResponseForEvent( GameEvent ev )
    {
      return _gameEventResponses->GetResponse(ev);
    }
    
  } // namespace Cozmo
} // namespace Anki
