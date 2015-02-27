//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

#include "robotMessageHandler.h"


namespace Anki {
  namespace Cozmo {
    
    RobotManager::RobotManager()
    {
      
    }
    
    void RobotManager::AddRobot(const RobotID_t withID, IRobotMessageHandler* msgHandler)
    {
      if(msgHandler == nullptr) {
        PRINT_NAMED_ERROR("RobotManager.AddRobot", "Can't add robot with null message handler.\n");
        return;
      }
      
      if (_robots.find(withID) == _robots.end()) {
        PRINT_NAMED_INFO("RobotManager.AddRobot", "Adding robot with ID=%d\n", withID);
        _robots[withID] = new Robot(withID, msgHandler);
        _IDs.push_back(withID);
      } else {
        PRINT_NAMED_WARNING("RobotManager.AddRobot.AlreadyAdded", "Robot with ID %d already exists. Ignoring.\n");
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

            CozmoEngineSignals::RobotDisconnectedSignal().emit(r->first, -1.f);
            
            r = _robots.erase(r);
            
            break;
          }
            
            // TODO: Handle other return results here
            
          default:
            ++r;
            break;
        }
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
