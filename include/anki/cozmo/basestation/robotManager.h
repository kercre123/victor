//
//  robot.h
//  Products_Cozmo
//
//     RobotManager class for keeping up with available robots, by their ID.
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef ANKI_COZMO_BASESTATION_ROBOTMANAGER_H
#define ANKI_COZMO_BASESTATION_ROBOTMANAGER_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include <map>
#include <vector>

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class Robot;
    class IMessageHandler;
    
    class RobotManager
    {
    public:
    
      RobotManager();

      // Sets pointers to other managers
      // TODO: Change these to interface pointers so they can't be NULL
      Result Init(IMessageHandler* msgHandler);
      
      // Get the list of known robot ID's
      std::vector<RobotID_t> const& GetRobotIDList() const;
      
      // Get a pointer to a robot by ID
      Robot* GetRobotByID(const RobotID_t robotID);
      
      // Check whether a robot exists
      bool DoesRobotExist(const RobotID_t withID) const;
      
      // Add / remove robots
      void AddRobot(const RobotID_t withID);
      void RemoveRobot(const RobotID_t withID);
      
      // Call each Robot's Update() function
      void UpdateAllRobots();
      
      // Return a
      // Return the number of availale robots
      size_t GetNumRobots() const;
      
    protected:
      
      IMessageHandler* _msgHandler;
      //BlockWorld*      _blockWorld;
      //IPathPlanner*    _pathPlanner;
      
      std::map<RobotID_t,Robot*> _robots;
      std::vector<RobotID_t>     _IDs;
      
    }; // class RobotManager
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOTMANAGER_H
