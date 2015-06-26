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
    class IRobotMessageHandler;
    
    class RobotManager
    {
    public:
    
      RobotManager();
      
      // Get the list of known robot ID's
      std::vector<RobotID_t> const& GetRobotIDList() const;

      // for when you don't care and you just want a damn robot
      Robot* GetFirstRobot();

      // Get a pointer to a robot by ID
      Robot* GetRobotByID(const RobotID_t robotID);
      
      // Check whether a robot exists
      bool DoesRobotExist(const RobotID_t withID) const;
      
      // Add / remove robots
      void AddRobot(const RobotID_t withID, IRobotMessageHandler* msgHandler);
      void RemoveRobot(const RobotID_t withID);
      
      // Call each Robot's Update() function
      void UpdateAllRobots();
      
      // Return a
      // Return the number of availale robots
      size_t GetNumRobots() const;
      
    protected:
      
      std::map<RobotID_t,Robot*> _robots;
      std::vector<RobotID_t>     _IDs;
      
    }; // class RobotManager
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOTMANAGER_H
