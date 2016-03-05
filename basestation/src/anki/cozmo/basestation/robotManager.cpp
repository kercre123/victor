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
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/fileUtils/fileUtils.h"
#include <sys/stat.h>

namespace Anki {
  namespace Cozmo {
    
    RobotManager::RobotManager(const CozmoContext* context)
    : _context(context)
    , _robotEventHandler(context)
    , _cannedAnimations(new CannedAnimationContainer())
    , _animationGroups(new AnimationGroupContainer())
    {
      ReadAnimationDir();
      ReadAnimationGroupDir();
    }
    
    RobotManager::~RobotManager() = default;
    
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
      if (_robots.find(robotID) != _robots.end()) {
        return _robots[robotID];
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

    
    void RobotManager::UpdateAllRobots()
    {
      //for (auto &r : _robots) {
      for(auto r = _robots.begin(); r != _robots.end(); ) {
        // Call update
        Robot* robot = r->second;
        Result result = robot->Update();
        
        switch(result)
        {
          case RESULT_FAIL_IO_TIMEOUT:
          {
            // Find the ID. This is inefficient, but this isn't a long list
            for(auto idIter = _IDs.begin(); idIter != _IDs.end(); ++idIter) {
              if(*idIter == r->first) {
                _IDs.erase(idIter);
                break;
              }
            }

            PRINT_NAMED_WARNING("RobotManager.UpdateAllRobots.FailIOTimeout", "Signaling robot disconnect\n");
            //CozmoEngineSignals::RobotDisconnectedSignal().emit(r->first);
            _robotDisconnectedSignal.emit(r->first);
            _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDisconnected(r->first, 0.0f)));
            
            delete r->second;
            r = _robots.erase(r);
            
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
                              r->first);
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
    }
    
    void RobotManager::ReadAnimationDirImpl(const std::string& animationDir)
    {
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
        if (mapIt == _loadedAnimationFiles.end()) {
          _loadedAnimationFiles.insert({path, attrib.st_mtimespec.tv_sec});
          loadFile = true;
        } else {
          if (mapIt->second < attrib.st_mtimespec.tv_sec) {
            mapIt->second = attrib.st_mtimespec.tv_sec;
            loadFile = true;
          } else {
            //PRINT_NAMED_INFO("Robot.ReadAnimationFile", "old time stamp for %s", fullFileName.c_str());
          }
        }
        if (loadFile) {
          ReadAnimationFile(path.c_str());
        }
      }
      
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
      
      auto filePaths = Util::FileUtils::FilesInDirectory(animationGroupFolder, true, "json");
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
        if (mapIt == _loadedAnimationGroupFiles.end()) {
          _loadedAnimationGroupFiles.insert({path, attrib.st_mtimespec.tv_sec});
          loadFile = true;
        } else {
          if (mapIt->second < attrib.st_mtimespec.tv_sec) {
            mapIt->second = attrib.st_mtimespec.tv_sec;
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
    
  } // namespace Cozmo
} // namespace Anki
