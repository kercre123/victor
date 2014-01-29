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

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class BlockWorld;
    class MatMarker2d;
    
    class Robot
    {
    public:
      enum OperationMode {
        MOVE_LIFT,
        FOLLOW_PATH,
        INITIATE_PRE_DOCK,
        DOCK
      };
      
      Robot(const RobotID_t robotID, BlockWorld& world);
      
      void Update();
      
      const std::list<Vision::ObservedMarker>& GetObservedVisionMarkers() const;
      
      /*
      // Add observed BlockMarker3d objects to a multimap container, grouped
      // by BlockType.
      // (It will be the world's job to take all of these from all
      //  robots and update the world state)
      void getVisibleBlockMarkers3d(std::multimap<BlockType, BlockMarker3d>& markers) ;
      */
      
      const RobotID_t get_ID() const;
      const Pose3d& get_pose() const;
      const Vision::Camera& get_camDown() const;
      const Vision::Camera& get_camHead() const;
      OperationMode get_operationMode() const;
      
      //const MatMarker2d* get_matMarker2d() const;
      
      //float get_downCamPixPerMM() const;
      
      void set_pose(const Pose3d &newPose);
      void set_headAngle(const Radians& angle);
      
      void queueIncomingMessage(const u8 *msg, const u8 msgSize);
      bool hasOutgoingMessages() const;
      void getOutgoingMessage(u8 *msgOut, u8 &msgSize);
      
      void dockWithBlock(const Block& block);
      
#define MESSAGE_DEFINITION_MODE MESSAGE_ROBOT_PROCESSOR_METHOD_MODE
#include "anki/cozmo/MessageDefinitions.h"
      
    protected:
      // The robot's identifier
      RobotID_t     ID_;
      
      // A reference to the BlockWorld the robot lives in
      BlockWorld*   world_;
      
      Pose3d pose;
      void updatePose();
      
      Vision::Camera camDown, camHead;
      bool camDownCalibSet, camHeadCalibSet;
      const Pose3d neckPose; // joint around which head rotates
      const Pose3d headCamPose; // in canonical (untilted) position w.r.t. neck joint
      const Pose3d liftBasePose; // around which the base rotates/lifts
      const Pose3d matCamPose; 
      Radians currentHeadAngle;
      
      
      OperationMode mode, nextMode;
      bool setOperationMode(OperationMode newMode);
      bool isCarryingBlock;
      
      const MatMarker2d            *matMarker;
      
      std::list<Vision::ObservedMarker>   observedVisionMarkers;
      //std::vector<BlockMarker3d*>  visibleFaces;
      //std::vector<Block*>          visibleBlocks;
      
      // TODO: compute this from down-camera calibration data
      float downCamPixPerMM = -1.f;
      
      // Message handling
      typedef std::vector<u8> MessageType;
      typedef std::queue<MessageType> MessageQueue;
      MessageQueue messagesIn;
      MessageQueue messagesOut;
      void checkMessages();
            
    }; // class Robot

    // Inline accessors:
    inline const RobotID_t Robot::get_ID(void) const
    { return this->ID_; }
    
    inline const Pose3d& Robot::get_pose(void) const
    { return this->pose; }
    
    inline const Vision::Camera& Robot::get_camDown(void) const
    { return this->camDown; }
    
    inline const Vision::Camera& Robot::get_camHead(void) const
    { return this->camHead; }
    
    inline Robot::OperationMode Robot::get_operationMode() const
    { return this->mode; }
    
    inline const std::list<Vision::ObservedMarker>& Robot::GetObservedVisionMarkers() const
    { return this->observedVisionMarkers; }
    
    /*
    inline const MatMarker2d* Robot::get_matMarker2d() const
    { return this->matMarker; }
    
    inline float Robot::get_downCamPixPerMM() const
    { return this->downCamPixPerMM; }
    */
    
    inline bool Robot::hasOutgoingMessages() const
    { return not this->messagesOut.empty(); }
    
    
    // Note that this is a singleton
    class RobotManager
    {
    public:
      
      // Return singleton instance
      static RobotManager* getInstance();
      
      // Get the list of known robot ID's
      std::vector<RobotID_t> const& GetRobotIDList() const;
      
      // Get a pointer to a robot by ID
      Robot* GetRobotByID(const RobotID_t robotID);
      
      // Check whether a robot exists
      bool DoesRobotExist(const RobotID_t withID) const;
      
      // Add / remove robots
      void AddRobot(const RobotID_t withID, BlockWorld& toWorld);
      void RemoveRobot(const RobotID_t withID);
      
      // Call each Robot's Update() function
      void UpdateAllRobots();
      
      // Return a
      // Return the number of availale robots
      size_t GetNumRobots() const;
      
      
      void GetAllVisionMarkers(std::vector<Vision::Marker>& markers) const;
      
    protected:
      
      RobotManager(); // protected constructor for singleton class
      
      static RobotManager* singletonInstance_;
      
      std::map<RobotID_t,Robot*> robots_;
      std::vector<RobotID_t>     ids_;
      
    }; // class RobotManager
    
    inline RobotManager* RobotManager::getInstance()
    {
      if(0 == singletonInstance_) {
        singletonInstance_ = new RobotManager();
      }
      return singletonInstance_;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot__
