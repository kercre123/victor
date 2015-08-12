//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotMessageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"


namespace Anki {
  namespace Cozmo {
    
    RobotManager::RobotManager(IExternalInterface* externalInterface)
    : _externalInterface(externalInterface)
    , _robotEventHandler(*this, externalInterface)
    {
      
    }
    
    void RobotManager::AddRobot(const RobotID_t withID, IRobotMessageHandler* msgHandler)
    {
      if(msgHandler == nullptr) {
        PRINT_NAMED_ERROR("RobotManager.AddRobot", "Can't add robot with null message handler.\n");
        return;
      }
      
      if (_robots.find(withID) == _robots.end()) {
        PRINT_STREAM_INFO("RobotManager.AddRobot", "Adding robot with ID=" << withID);
        _robots[withID] = new Robot(withID, msgHandler, _externalInterface);
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
        Result result = r->second->Update();
        
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
            _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDisconnected(r->first, 0.0f)));
            
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
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
