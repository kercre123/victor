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

#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/camera.h"
#include "anki/cozmo/basestation/block.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class BlockWorld;
    class MatMarker2d;
    
#if 0
    // TOOD: Is this cozmo-specific? Should it be in Coretech-Communications?
    class RobotMessage
    {
    public:
      enum MessageType {
        BlockMarkerObservation = 1,
        MatMarkerObservation   = 2
      };
      
      RobotMessage(MessageType type, const char* buffer);
      
      MessageType get_type() const;
      
    protected:
      MessageType type;
      
      void *data;
      
      // TODO: Have some kind of MessageTranslator member that inherits from
      //       someething in coretech-communications?
      
    }; // class RobotMessage
    
    
    class BlockObservationMessage : public RobotMessage
    {
    public:
      BlockObservationMessage(const CozmoMsg_ObservedBlockMarker *msg);
      
    private:
      u16 blockType;
      u8  faceType;
      
      Quad2f
      
    };
#endif
    
    class Robot
    {
    public:
      enum OperationMode {
        MOVE_LIFT,
        FOLLOW_PATH,
        INITIATE_PRE_DOCK,
        DOCK
      };
      
      Robot();
      //Robot(BlockWorld &theWorld);
      
      void step();
      
      // Add observed BlockMarker3d objects to the list:
      // (It will be the world's job to take all of these from all
      //  robots and update the world state)
      void getVisibleBlockMarkers3d(std::vector<BlockMarker3d> &markers) const;
      
      const u8 &get_ID() const;
      const Pose3d& get_pose() const;
      const Camera& get_camDown() const;
      const Camera& get_camHead() const;
      OperationMode get_operationMode() const;
      const MatMarker2d* get_matMarker2d() const;
      
      float get_downCamPixPerMM() const;
      
      void set_pose(const Pose3d &newPose);
      
      void queueMessage(const u8 *msg, const u8 msgSize);
      
    protected:
      u8 ID;
      
      Camera camDown, camHead;
      
      Pose3d pose;
      
      //BlockWorld &world;
      
      OperationMode mode, nextMode;
      bool setOperationMode(OperationMode newMode);
      
      const MatMarker2d            *matMarker2d;
      
      std::vector<BlockMarker2d>   visibleBlockMarkers2d;
      //std::vector<BlockMarker3d*>  visibleFaces;
      //std::vector<Block*>          visibleBlocks;
      
      // TODO: compute this from down-camera calibration data
      float downCamPixPerMM;
      
      typedef std::vector<u8> MessageType;
      typedef std::queue<MessageType> MessageQueue;
      MessageQueue messages;
      //void checkMessages(std::queue<RobotMessage> *messageQueue);
      
      template<typename MSG_TYPE>
      void processMessage(const MSG_TYPE *msg);
      
    }; // class Robot

    // Inline accessors:
    inline const u8& Robot::get_ID(void) const
    { return this->ID; }
    
    inline const Pose3d& Robot::get_pose(void) const
    { return this->pose; }
    
    inline const Camera& Robot::get_camDown(void) const
    { return this->camDown; }
    
    inline const Camera& Robot::get_camHead(void) const
    { return this->camHead; }
    
    inline Robot::OperationMode Robot::get_operationMode() const
    { return this->mode; }
    
    inline const MatMarker2d* Robot::get_matMarker2d() const
    { return this->matMarker2d; }
    
    inline float Robot::get_downCamPixPerMM() const
    { return this->downCamPixPerMM; }
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot__
