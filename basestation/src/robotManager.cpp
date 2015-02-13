//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

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
        _robots[withID] = new Robot(withID, msgHandler);
        _IDs.push_back(withID);
      } else {
        PRINT_NAMED_WARNING("RobotManager.AddRobot.AlreadyAdded", "Robot with ID %d already exists. Ignoring.\n");
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
      for (auto &r : _robots) {
        // Call update
        r.second->Update();
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
