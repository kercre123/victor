//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "anki/cozmo/basestation/events/animationTriggerResponsesContainer.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/firmwareUpdater/firmwareUpdater.h"
#include "anki/cozmo/basestation/robotInitialConnection.h"
#include "anki/common/robot/config.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "json/json.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/time/stepTimers.h"
#include <vector>
#include <sys/stat.h>

#include "anki/common/robot/config.h"
#include "util/global/globalDefinitions.h"
#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"
#endif

namespace Anki {
  namespace Cozmo {
    
    RobotManager::RobotManager(const CozmoContext* context)
    : _context(context)
    , _robotEventHandler(context)
    , _cannedAnimations(context->GetDataLoader()->GetCannedAnimations())
    , _animationGroups(context->GetDataLoader()->GetAnimationGroups())
    , _animationTriggerResponses(context->GetDataLoader()->GetAnimationTriggerResponses())
    , _firmwareUpdater(new FirmwareUpdater(context))
    , _robotMessageHandler(new RobotInterface::MessageHandler())
    , _fwVersion(0)
    , _fwTime(0)
    {
      using namespace ExternalInterface;
      
      auto broadcastAvailableAnimationsCallback = [this](const AnkiEvent<MessageGameToEngine>& event)
      {
        this->BroadcastAvailableAnimations();
      };
      auto broadcastAvailableAnimationGroupsCallback = [this](const AnkiEvent<MessageGameToEngine>& event)
      {
        this->BroadcastAvailableAnimationGroups();
      };
        
      IExternalInterface* externalInterface = context->GetExternalInterface();

      MessageGameToEngineTag tagAnims = MessageGameToEngineTag::RequestAvailableAnimations;
      MessageGameToEngineTag tagGroups = MessageGameToEngineTag::RequestAvailableAnimationGroups;
        
      if (externalInterface != nullptr){
        _signalHandles.push_back( externalInterface->Subscribe(tagGroups, broadcastAvailableAnimationGroupsCallback) );
        _signalHandles.push_back( externalInterface->Subscribe(tagAnims, broadcastAvailableAnimationsCallback) );
      }
    }
    
    RobotManager::~RobotManager() = default;
    
    void RobotManager::Init(const Json::Value& config)
    {
      auto startTime = std::chrono::system_clock::now();
    
      Anki::Util::Time::PushTimedStep("RobotManager::Init");
      _robotMessageHandler->Init(config, this, _context);
      Anki::Util::Time::PopTimedStep(); // RobotManager::Init
      
      Anki::Util::Time::PrintTimedSteps();
      Anki::Util::Time::ClearSteps();

      BroadcastAvailableAnimations();
      
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

      _firmwareUpdater->LoadHeader(std::bind(&RobotManager::ParseFirmwareHeader, this, std::placeholders::_1));
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      if (_robots.find(withID) == _robots.end()) {
        PRINT_STREAM_INFO("RobotManager.AddRobot", "Adding robot with ID=" << withID);
        _robots[withID] = new Robot(withID, _context);
        _IDs.push_back(withID);
        _initialConnections.emplace(std::piecewise_construct,
          std::forward_as_tuple(withID),
          std::forward_as_tuple(withID, _robotMessageHandler.get(), _context->GetExternalInterface(), _fwVersion, _fwTime));
      } else {
        PRINT_STREAM_WARNING("RobotManager.AddRobot.AlreadyAdded", "Robot with ID " << withID << " already exists. Ignoring.");
      }
      
      if (_context->GetExternalInterface())
      {
        _robotEventHandler.SetupGainsHandlers(*(_context->GetExternalInterface()));
        _robotEventHandler.SetupMiscHandlers(*(_context->GetExternalInterface()));
      }
    }
    
    void RobotManager::RemoveRobot(const RobotID_t withID)
    {
      auto iter = _robots.find(withID);
      if(iter != _robots.end()) {
        PRINT_NAMED_INFO("RobotManager.RemoveRobot", "Removing robot with ID=%d\n", withID);
        
        _robotDisconnectedSignal.emit(withID);
        // ask initial connection tracker if it's handling this
        bool handledDisconnect = false;
        auto initialIter = _initialConnections.find(withID);
        if (initialIter != _initialConnections.end()) {
          handledDisconnect = initialIter->second.HandleDisconnect();
        }
        if (!handledDisconnect) {
          _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDisconnected(withID, 0.0f)));
        }
        
        delete(iter->second);
        iter = _robots.erase(iter);
        
        // Find the ID. This is inefficient, but this isn't a long list
        for(auto idIter = _IDs.begin(); idIter != _IDs.end(); ++idIter) {
          if(*idIter == withID) {
            _IDs.erase(idIter);
            break;
          }
        }
        _initialConnections.erase(withID);
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
      ANKI_CPU_PROFILE("RobotManager::UpdateAllRobots");
      
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
    
    void RobotManager::UpdateRobotConnection()
    {
      ANKI_CPU_PROFILE("RobotManager::UpdateRobotConnection");
      _robotMessageHandler->ProcessMessages();
    }
    
    void RobotManager::ReadAnimationDir()
    {
      _context->GetDataLoader()->LoadAnimations();
      BroadcastAvailableAnimations();
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

        _context->GetExternalInterface()->
          BroadcastToGame<ExternalInterface::EndOfMessage>(ExternalInterface::MessageType::AnimationAvailable);
        PRINT_NAMED_DEBUG("RobotManager.BroadcastAvailableAnimations", "Supposedly sent EndOfMessage");
      }
    }
    
    void RobotManager::BroadcastAvailableAnimationGroups()
    {
      Anki::Util::Time::ScopedStep scopeTimer("BroadcastAvailableAnimationGroups");
      if (nullptr != _context->GetExternalInterface()) {
        std::vector<std::string> animNames(_animationGroups->GetAnimationGroupNames());
        for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
          _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationGroupAvailable>(*i);
        }
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
    bool RobotManager::HasAnimationForTrigger( AnimationTrigger ev )
    {
      return _animationTriggerResponses->HasResponse(ev);
    }
    std::string RobotManager::GetAnimationForTrigger( AnimationTrigger ev )
    {
      return _animationTriggerResponses->GetResponse(ev);
    }

    void RobotManager::ParseFirmwareHeader(const Json::Value& header)
    {
      JsonTools::GetValueOptional(header, FirmwareUpdater::kFirmwareVersionKey, _fwVersion);
      JsonTools::GetValueOptional(header, FirmwareUpdater::kFirmwareTimeKey, _fwTime);
      if (_fwVersion == 0 || _fwTime == 0) {
        PRINT_NAMED_WARNING("RobotManager.ParseFirmwareHeader", "got version %d, time %d", _fwVersion, _fwTime);
      }
    }

    bool RobotManager::ShouldFilterMessage(const RobotID_t robotId, const RobotInterface::RobotToEngineTag msgType) const
    {
      auto iter = _initialConnections.find(robotId);
      if (iter == _initialConnections.end()) {
        return false;
      }
      return iter->second.ShouldFilterMessage(msgType);
    }

    bool RobotManager::ShouldFilterMessage(const RobotID_t robotId, const RobotInterface::EngineToRobotTag msgType) const
    {
      auto iter = _initialConnections.find(robotId);
      if (iter == _initialConnections.end()) {
        return false;
      }
      return iter->second.ShouldFilterMessage(msgType);
    }
    
  } // namespace Cozmo
} // namespace Anki
