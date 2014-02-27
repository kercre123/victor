//
//  robot.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__robot__
#define __Products_Cozmo__robot__

#include <queue>

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/visionMarker.h"

#include "anki/planning/shared/path.h"

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class BlockWorld;
    class MessageHandler;
    
    class Robot
    {
    public:
      enum OperationMode {
        MOVE_LIFT,
        FOLLOW_PATH,
        INITIATE_PRE_DOCK,
        DOCK
      };
      
      Robot(const RobotID_t robotID, MessageHandler* msgHandler, BlockWorld* world);
      
      void Update();
      
      const RobotID_t get_ID() const;
      const Pose3d& get_pose() const;
      const Vision::Camera& get_camDown() const;
      const Vision::Camera& get_camHead() const;
      OperationMode get_operationMode() const;
      
      void set_pose(const Pose3d &newPose);
      void set_headAngle(const Radians& angle);
      void set_camCalibration(const Vision::CameraCalibration& calib);
      
      void queueIncomingMessage(const u8 *msg, const u8 msgSize);
      bool hasOutgoingMessages() const;
      void getOutgoingMessage(u8 *msgOut, u8 &msgSize);
      
      void dockWithBlock(const Block& block);
      
    protected:
      // The robot's identifier
      RobotID_t     ID_;
      
      // A reference to the MessageHandler that the robot uses for outgoing comms
      MessageHandler* msgHandler_;
      
      // A reference to the BlockWorld the robot lives in
      BlockWorld*   world_;
      
      Pose3d pose;
      void updatePose();
      
      Vision::Camera camHead;

      const Pose3d neckPose; // joint around which head rotates
      const Pose3d headCamPose; // in canonical (untilted) position w.r.t. neck joint
      const Pose3d liftBasePose; // around which the base rotates/lifts

      Radians currentHeadAngle;
      
      OperationMode mode, nextMode;
      bool setOperationMode(OperationMode newMode);
      bool isCarryingBlock;
      
      //std::vector<BlockMarker3d*>  visibleFaces;
      //std::vector<Block*>          visibleBlocks;
      
      // Message handling
      typedef std::vector<u8> MessageType;
      typedef std::queue<MessageType> MessageQueue;
      MessageQueue messagesOut;
      
            
    }; // class Robot

    //
    // Inline accessors:
    //
    inline const RobotID_t Robot::get_ID(void) const
    { return this->ID_; }
    
    inline const Pose3d& Robot::get_pose(void) const
    { return this->pose; }
    
    inline const Vision::Camera& Robot::get_camHead(void) const
    { return this->camHead; }
    
    inline Robot::OperationMode Robot::get_operationMode() const
    { return this->mode; }
    
    inline void Robot::set_camCalibration(const Vision::CameraCalibration& calib)
    { this->camHead.set_calibration(calib); }
    
    inline bool Robot::hasOutgoingMessages() const
    { return not this->messagesOut.empty(); }
    
    
    //
    // RobotManager class for keeping up with available robots, by their ID
    //
    // TODO: Singleton or not?
#define USE_SINGLETON_ROBOT_MANAGER 0
    
    class RobotManager
    {
    public:
    
#if USE_SINGLETON_ROBOT_MANAGER
      // Return singleton instance
      static RobotManager* getInstance();
#else
      RobotManager();
#endif

      // Sets pointers to other managers
      ReturnCode Init(MessageHandler* msgHandler, BlockWorld* blockWorld);
      
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
      
#if USE_SINGLETON_ROBOT_MANAGER
      RobotManager(); // protected constructor for singleton class
      
      static RobotManager* singletonInstance_;
#endif
      
      MessageHandler* msgHandler_;
      BlockWorld* blockWorld_;
      
      std::map<RobotID_t,Robot*> robots_;
      std::vector<RobotID_t>     ids_;
      
    }; // class RobotManager
    
#if USE_SINGLETON_ROBOT_MANAGER
    inline RobotManager* RobotManager::getInstance()
    {
      if(0 == singletonInstance_) {
        singletonInstance_ = new RobotManager();
      }
      return singletonInstance_;
    }
#endif
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot__
