#include "anki/cozmo/robot/messages.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/visionSystem.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
#include "liftController.h"
#include "headController.h"
#include "dockingController.h"
#include "pickAndPlaceController.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {
      
      namespace {
  
        // 4. Fill in the message information lookup table:
        typedef struct {
          u8 priority;
          u8 size;
          void (*dispatchFcn)(const u8* buffer);
        } TableEntry;
        
        const size_t NUM_TABLE_ENTRIES = NUM_MSG_IDS + 1;
        const TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
          {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
          {0, 0, 0} // Final dummy entry without comma at end
        };
        

        u8 msgBuffer_[256];
        
        // For waiting for a particular message ID
        const u32 LOOK_FOR_MESSAGE_TIMEOUT = 1000000;
        ID lookForID_ = NO_MESSAGE_ID;
        u32 lookingStartTime_ = 0;
        
        //
        // Mailboxes
        //
        //   "Mailboxes" are used for leaving messages from slower
        //   vision processing in LongExecution to be retrieved and acted upon
        //   by the faster MainExecution.
        //
        
        // Single-message Mailbox Class
        template<typename MsgType>
        class Mailbox
        {
        public:
          
          Mailbox();
          
          bool putMessage(const MsgType newMsg);
          bool getMessage(MsgType& msgOut);
          
        protected:
          MsgType message_;
          bool    beenRead_;
          bool    isLocked_;
        };
        
        // Multiple-message Mailbox Class
        template<typename MSG_TYPE, u8 NUM_BOXES>
        class MultiMailbox
        {
        public:
        
          bool putMessage(const MSG_TYPE newMsg);
          bool getMessage(MSG_TYPE& msg);
        
        protected:
          Mailbox<MSG_TYPE> mailboxes_[NUM_BOXES];
          u8 readIndex_, writeIndex_;
          
          void advanceIndex(u8 &index);
        };
        
        
        // Mailboxes for different types of messages that the vision
        // system communicates to main execution:
        //MultiMailbox<Messages::BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> blockMarkerMailbox_;
        //Mailbox<Messages::MatMarkerObserved>    matMarkerMailbox_;
        Mailbox<Messages::DockingErrorSignal>   dockingMailbox_;

        static RobotState robotState_;
        
      } // private namespace
      

// #pragma mark --- Messages Method Implementations ---
      
      u8 GetSize(const ID msgID)
      {
        return LookupTable_[msgID].size;
      }
      
      void ProcessMessage(const ID msgID, const u8* buffer)
      {
        if(LookupTable_[msgID].dispatchFcn != NULL) {
          PRINT("ProcessMessage(): Dispatching message with ID=%d.\n", msgID);
          
          (*LookupTable_[msgID].dispatchFcn)(buffer);
        }
        
        if(lookForID_ != NO_MESSAGE_ID) {
          
          // See if this was the message we were told to look for
          if(msgID == lookForID_) {
            lookForID_ = NO_MESSAGE_ID;
          }
        }
      } // ProcessMessage()
      
      void LookForID(const ID msgID) {
        lookForID_ = msgID;
        lookingStartTime_ = HAL::GetMicroCounter();
      }
      
      bool StillLookingForID(void) {
        if(lookForID_ == NO_MESSAGE_ID) {
          return false;
        }
        else if(HAL::GetMicroCounter() - lookingStartTime_ > LOOK_FOR_MESSAGE_TIMEOUT) {
            PRINT("Timed out waiting for message ID %d.\n", lookForID_);
            lookForID_ = NO_MESSAGE_ID;
          
          return false;
        }
        
        return true;
        
      }
      
      RobotState const& GetRobotStateMsg() {
        return robotState_;
      }
      
      
// #pragma --- Message Dispatch Functions ---
      
      
      void ProcessRobotAddedToWorldMessage(const RobotAddedToWorld& msg)
      {
        if(msg.robotID != HAL::GetRobotID()) {
          PRINT("Robot received ADDED_TO_WORLD handshake with "
                " wrong robotID (%d instead of %d).\n",
                msg.robotID, HAL::GetRobotID());
        }
        
        PRINT("Robot received handshake from basestation, "
              "sending camera calibration.\n");
        const HAL::CameraInfo *headCamInfo = HAL::GetHeadCamInfo();
        
        //
        // Send head camera calibration
        //
        //   TODO: do we send x or y focal length, or both?
        HeadCameraCalibration headCalibMsg = {
          headCamInfo->focalLength_x,
          headCamInfo->focalLength_y,
          headCamInfo->center_x,
          headCamInfo->center_y,
          headCamInfo->skew,
          headCamInfo->nrows,
          headCamInfo->ncols};
        
        HAL::RadioSendMessage(GET_MESSAGE_ID(HeadCameraCalibration), &headCalibMsg);
        
      } // ProcessRobotAddedMessage()
      
      
      void ProcessAbsLocalizationUpdateMessage(const AbsLocalizationUpdate& msg)
      {
        // TODO: Double-check that size matches expected size?
        
        // TODO: take advantage of timestamp
        
        f32 currentMatX       = msg.xPosition;
        f32 currentMatY       = msg.yPosition;
        Radians currentMatHeading = msg.headingAngle;
        Localization::SetCurrentMatPose(currentMatX, currentMatY, currentMatHeading);
        
        PRINT("Robot received localization update from "
              "basestation: (%.3f,%.3f) at %.1f degrees\n",
              currentMatX, currentMatY,
              currentMatHeading.getDegrees());
#if(USE_OVERLAY_DISPLAY)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_POSE, "Pose: (x,y)=(%.4f,%.4f) at angle=%.1f\n",
                  currentMatX, currentMatY,
                  currentMatHeading.getDegrees());
        }
#endif
        
      } // ProcessAbsLocalizationUpdateMessage()
      
      
      void ProcessRequestCamCalibMessage(const RequestCamCalib& msg)
      {
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();
        if(headCamInfo == NULL) {
          PRINT("NULL HeadCamInfo retrieved from HAL.\n");
        }
        else {
          Messages::HeadCameraCalibration headCalibMsg = {
            headCamInfo->focalLength_x,
            headCamInfo->focalLength_y,
            headCamInfo->center_x,
            headCamInfo->center_y,
            headCamInfo->skew,
            headCamInfo->nrows,
            headCamInfo->ncols
          };
          
          
          if(!HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::HeadCameraCalibration),
                                   &headCalibMsg))
          {
            PRINT("Failed to send camera calibration message.\n");
          }
        }

      }
      
      void ProcessDockingErrorSignalMessage(const DockingErrorSignal& msg)
      {
        // Just pass the docking error signal along to the mainExecution to
        // deal with. Note that if the message indicates tracking failed,
        // the mainExecution thread should handle it, and put the vision
        // system back in LOOKING_FOR_BLOCKS mode.
        dockingMailbox_.putMessage(msg);
      }
      
      void ProcessBTLEMessages()
      {
        ID msgID;
        
        while((msgID = HAL::RadioGetNextMessage(msgBuffer_)) != NO_MESSAGE_ID)
        {
          ProcessMessage(msgID, msgBuffer_);
        }
        
      } // ProcessBTLEMessages()
      
      void ProcessClearPathMessage(const ClearPath& msg) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        //SteeringController::ExecuteDirectDrive(0,0);
      }

      void ProcessAppendPathSegmentArcMessage(const AppendPathSegmentArc& msg) {
        PathFollower::AppendPathSegment_Arc(0, msg.x_center_mm, msg.y_center_mm, msg.radius_mm, msg.startRad, msg.sweepRad,
                                            msg.targetSpeed, msg.accel, msg.decel);
      }
      
      void ProcessAppendPathSegmentLineMessage(const AppendPathSegmentLine& msg) {
        PathFollower::AppendPathSegment_Line(0, msg.x_start_mm, msg.y_start_mm, msg.x_end_mm, msg.y_end_mm,
                                             msg.targetSpeed, msg.accel, msg.decel);
      }
      
      void ProcessExecutePathMessage(const ExecutePath& msg) {
        PathFollower::StartPathTraversal();
      }
      
      void ProcessDockWithBlockMessage(const DockWithBlock& msg)
      {
        DockingController::StartDocking(static_cast<Vision::MarkerType>(msg.markerType),
                                        msg.markerWidth_mm,
                                        msg.horizontalOffset_mm,
                                        0, 0);
      }

      void ProcessDriveWheelsMessage(const DriveWheels& msg) {
        //PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
      }
      
      void ProcessDriveWheelsCurvatureMessage(const DriveWheelsCurvature& msg) {
        /*
        PathFollower::ClearPath();
        
        SpeedController::SetUserCommandedDesiredVehicleSpeed(msg.speed_mmPerSec);
        SpeedController::SetUserCommandedAcceleration(msg.accel_mmPerSec2);
        SpeedController::SetUserCommandedDeceleration(msg.decel_mmPerSec2);
        */
      }
      
      void ProcessMoveLiftMessage(const MoveLift& msg) {
        LiftController::SetSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        LiftController::SetDesiredHeight(msg.height_mm);
      }
      
      void ProcessMoveHeadMessage(const MoveHead& msg) {
        HeadController::SetSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        HeadController::SetDesiredAngle(msg.angle_rad);
      }
      
      void ProcessStopAllMotorsMessage(const StopAllMotors& msg) {
        SteeringController::ExecuteDirectDrive(0,0);
        LiftController::SetAngularVelocity(0);
        HeadController::SetAngularVelocity(0);
      }
      
      void ProcessHeadAngleUpdateMessage(const HeadAngleUpdate& msg) {
        HeadController::SetAngleRad(msg.newAngle);
      }
      
      void ProcessImageRequestMessage(const ImageRequest& msg) {
        VisionSystem::SendNextImage((Vision::CameraResolution)msg.resolution);
      }
      
      
      // TODO: Fill these in once they are needed/used:
      void ProcessVisionMarkerMessage(const VisionMarker& msg) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessBlockMarkerObservedMessage(const BlockMarkerObserved& msg) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessMatMarkerObservedMessage(const MatMarkerObserved& msg) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessRobotAvailableMessage(const RobotAvailable& msg) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      // These need implementations to avoid linker errors, but we don't expect
      // to _receive_ these message types, only to send them.
      void ProcessMatCameraCalibrationMessage(const MatCameraCalibration& msg) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessHeadCameraCalibrationMessage(const HeadCameraCalibration& msg) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessRobotStateMessage(const RobotState& msg) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }

      void ProcessPrintTextMessage(const PrintText& msg) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessImageChunkMessage(const ImageChunk& msg) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
// ----------- Send messages -----------------
      
      
      Result SendRobotStateMsg()
      {
        robotState_.timestamp = HAL::GetTimeStamp();
        
        Radians poseAngle;
        
        Localization::GetCurrentMatPose(robotState_.pose_x, robotState_.pose_y, poseAngle);
        robotState_.pose_z = 0;
        robotState_.pose_angle = poseAngle.ToFloat();
        
        WheelController::GetFilteredWheelSpeeds(robotState_.lwheel_speed_mmps, robotState_.rwheel_speed_mmps);

        robotState_.headAngle  = HeadController::GetAngleRad();
        robotState_.liftHeight = LiftController::GetHeightMM();

        robotState_.isTraversingPath = PathFollower::IsTraversingPath() ? 1 : 0;
        robotState_.isCarryingBlock = PickAndPlaceController::IsCarryingBlock() ? 1 : 0;
        
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::RobotState), &robotState_) == true) {
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      }
      
      
      void SendText(const char *format, ...)
      {
        #define MAX_SEND_TEXT_LENGTH 512
        char text[MAX_SEND_TEXT_LENGTH];
        memset(text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        va_list argptr;
        va_start(argptr, format);
        vsnprintf(text, MAX_SEND_TEXT_LENGTH, format, argptr);
        va_end(argptr);
        
        // Breakup and send in multiple messages if necessary
        Messages::PrintText m;
        s32 bytesLeftToSend = strlen(text);
        u8 numMsgs = 0;
        while(bytesLeftToSend > 0) {
          memset(m.text, 0, PRINT_TEXT_MSG_LENGTH);
          u32 currPacketBytes = MIN(PRINT_TEXT_MSG_LENGTH, bytesLeftToSend);
          memcpy(m.text, text + numMsgs*PRINT_TEXT_MSG_LENGTH, currPacketBytes);
          
          bytesLeftToSend -= PRINT_TEXT_MSG_LENGTH;
          
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::PrintText), &m);
          numMsgs++;
        }
      }
      
      
// #pragma mark --- VisionSystem::Mailbox Template Implementations ---
      
      /*
      bool CheckMailbox(BlockMarkerObserved& msg)
      {
        return blockMarkerMailbox_.getMessage(msg);
      }
      
      bool CheckMailbox(MatMarkerObserved&   msg)
      {
        return matMarkerMailbox_.getMessage(msg);
      }
       */
      
      bool CheckMailbox(DockingErrorSignal&  msg)
      {
        return dockingMailbox_.getMessage(msg);
      }
      
      //
      // Templated Mailbox Implementations
      //
      template<typename MSG_TYPE>
      Mailbox<MSG_TYPE>::Mailbox()
      : beenRead_(true)
      {
      
      }
      
      template<typename MSG_TYPE>
      bool Mailbox<MSG_TYPE>::putMessage(const MSG_TYPE newMsg) {
        if(isLocked_) {
          return false;
        }
        else {
          isLocked_ = true;    // Lock
          message_  = newMsg;
          beenRead_ = false;
          isLocked_ = false;   // Unlock
          return true;
        }
      }
      
      template<typename MSG_TYPE>
      bool Mailbox<MSG_TYPE>::getMessage(MSG_TYPE& msgOut) {
        if(isLocked_ || beenRead_) {
          return false;
        }
        else {
          isLocked_ = true;   // Lock
          msgOut = message_;
          beenRead_ = true;
          isLocked_ = false;  // Unlock
          return true;
        }
      }
      
      
      //
      // Templated MultiMailbox Implementations
      //
      
      template<typename MSG_TYPE, u8 NUM_BOXES>
      bool MultiMailbox<MSG_TYPE,NUM_BOXES>::putMessage(const MSG_TYPE newMsg) {
        if(mailboxes_[writeIndex_].putMessage(newMsg) == true) {
          advanceIndex(writeIndex_);
          return true;
        }
        else {
          return false;
        }
      }
      
      template<typename MSG_TYPE, u8 NUM_BOXES>
      bool MultiMailbox<MSG_TYPE,NUM_BOXES>::getMessage(MSG_TYPE& msg) {
        if(mailboxes_[readIndex_].getMessage(msg) == true) {
          // we got a message out of the mailbox (it wasn't locked and there
          // was something in it), so move to the next mailbox
          advanceIndex(readIndex_);
          return true;
        }
        else {
          return false;
        }
      }
   
      template<typename MSG_TYPE, u8 NUM_BOXES>
      void MultiMailbox<MSG_TYPE,NUM_BOXES>::advanceIndex(u8 &index) {
        ++index;
        if(index == NUM_BOXES) {
          index = 0;
        }
      }

      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
