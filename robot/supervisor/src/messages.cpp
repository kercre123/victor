#include "anki/cozmo/messages.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/visionSystem.h"

#include "anki/common/shared/radians.h"

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
        
        // TODO: Would be nice to use NUM_MSG_IDS instead of hard-coded 256 here.
        TableEntry LookupTable_[256] = {
          {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
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
        
        const u8 MAX_BLOCK_MARKER_MESSAGES = 32; // max blocks we can see in one image
        
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
        MultiMailbox<Messages::BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> blockMarkerMailbox_;
        Mailbox<Messages::MatMarkerObserved>    matMarkerMailbox_;
        Mailbox<Messages::DockingErrorSignal>   dockingMailbox_;
        
      } // private namespace
      

#pragma mark --- Messages Method Implementations ---
      
      u8 GetSize(const ID msgID)
      {
        return LookupTable_[msgID].size;
      }
      
      void ProcessMessage(const ID msgID, const u8* buffer)
      {
        if(LookupTable_[msgID].dispatchFcn != NULL) {
          
          (*LookupTable_[msgID].dispatchFcn)(buffer);
          
          PRINT("ProcessMessage(): Dispatching message with ID=%d.\n", msgID);
        }
        
        if(lookForID_ != NO_MESSAGE_ID) {
          
          // See if this was the message we were told to look for
          if(msgID == lookForID_) {
            lookForID_ = NO_MESSAGE_ID;
          }
          else if(HAL::GetMicroCounter() - lookingStartTime_ > LOOK_FOR_MESSAGE_TIMEOUT) {
            PRINT("Timed out waiting for message ID %d.\n", lookForID_);
            lookForID_ = NO_MESSAGE_ID;
          }
        }
      } // ProcessMessage()
      
      void LookForID(const ID msgID) {
        lookForID_ = msgID;
        lookingStartTime_ = HAL::GetMicroCounter();
      }
      
      bool StillLookingForID(void) {
        return lookForID_ != NO_MESSAGE_ID;
      }
      
      
#pragma --- Message Dispatch Functions ---
      
      void ProcessRobotAddedToWorldMessage(const u8* buffer)
      {
        const RobotAddedToWorld* msg = reinterpret_cast<const RobotAddedToWorld*>(buffer);
        
        if(msg->robotID != HAL::GetRobotID()) {
          PRINT("Robot received ADDED_TO_WORLD handshake with "
                " wrong robotID (%d instead of %d).\n",
                msg->robotID, HAL::GetRobotID());
        }
        
        PRINT("Robot received handshake from basestation, "
              "sending camera calibration.\n");
        const HAL::CameraInfo *matCamInfo  = HAL::GetMatCamInfo();
        const HAL::CameraInfo *headCamInfo = HAL::GetHeadCamInfo();
        
        //
        // Send mat camera calibration
        //
        //   TODO: do we send x or y focal length, or both?
        MatCameraCalibration matCalibMsg = {
          matCamInfo->focalLength_x,
          matCamInfo->focalLength_y,
          matCamInfo->fov_ver,
          matCamInfo->center_x,
          matCamInfo->center_y,
          matCamInfo->skew,
          matCamInfo->nrows,
          matCamInfo->ncols};
        
        HAL::RadioSendMessage(GET_MESSAGE_ID(MatCameraCalibration), &matCalibMsg);
        
        //
        // Send head camera calibration
        //
        //   TODO: do we send x or y focal length, or both?
        HeadCameraCalibration headCalibMsg = {
          headCamInfo->focalLength_x,
          headCamInfo->focalLength_y,
          headCamInfo->fov_ver,
          headCamInfo->center_x,
          headCamInfo->center_y,
          headCamInfo->skew,
          headCamInfo->nrows,
          headCamInfo->ncols};
        
        HAL::RadioSendMessage(GET_MESSAGE_ID(HeadCameraCalibration), &headCalibMsg);
        
      } // ProcessRobotAddedMessage()
      
      
      void ProcessAbsLocalizationUpdateMessage(const u8* buffer)
      {
        // TODO: Double-check that size matches expected size?
        
        const AbsLocalizationUpdate *msg = reinterpret_cast<const AbsLocalizationUpdate*>(buffer);
        
        f32 currentMatX       = msg->xPosition * .001f; // store in meters
        f32 currentMatY       = msg->yPosition * .001f; //     "
        Radians currentMatHeading = msg->headingAngle;
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
      
      
      void ProcessBlockMarkerObservedMessage(const u8* buffer)
      {
        PRINT("Processing BlockMarker message\n");
        
        const BlockMarkerObserved* msg = reinterpret_cast<const BlockMarkerObserved*>(buffer);
        
        blockMarkerMailbox_.putMessage(*msg);
        
        VisionSystem::CheckForDockingBlock(msg->blockType);
      }
      
      void ProcessTotalBlocksDetectedMessage(const u8* buffer)
      {
        const TotalBlocksDetected* msg = reinterpret_cast<const TotalBlocksDetected*>(buffer);
        
        PRINT("Saw %d block markers.\n", msg->numBlocks);
      }
      
      void ProcessTemplateInitializedMessage(const u8* buffer)
      {
        const TemplateInitialized* msg = reinterpret_cast<const TemplateInitialized*>(buffer);
        
        VisionSystem::SetDockingMode(static_cast<bool>(msg->success));
        
      }
      
      void ProcessDockingErrorSignalMessage(const u8* buffer)
      {
        const DockingErrorSignal* msg = reinterpret_cast<const DockingErrorSignal*>(buffer);
        
        VisionSystem::UpdateTrackingStatus(msg->didTrackingSucceed);
        
        // Just pass the docking error signal along to the mainExecution to
        // deal with. Note that if the message indicates tracking failed,
        // the mainExecution thread should handle it, and put the vision
        // system back in LOOKING_FOR_BLOCKS mode.
        dockingMailbox_.putMessage(*msg);
      }
      
      void ProcessBTLEMessages()
      {
        ID msgID;
        
        while((msgID = HAL::RadioGetNextMessage(msgBuffer_)) != NO_MESSAGE_ID)
        {
          ProcessMessage(msgID, msgBuffer_);
        }
        
      } // ProcessBTLEMessages()
      
      void ProcessUARTMessages()
      {
        ID msgID;
        
        while((msgID = HAL::USBGetNextMessage(msgBuffer_)) != NO_MESSAGE_ID)
        {
          ProcessMessage(msgID, msgBuffer_);
        }
        
      } // ProcessUARTMessages()
      
      
      // TODO: Fill these in once they are needed/used:
      void ProcessClearPathMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetMotionMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessRobotAvailableMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessMatMarkerObservedMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetPathSegmentArcMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetPathSegmentLineMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      // These need implementations to avoid linker errors, but we don't expect
      // to _receive_ these message types, only to send them.
      void ProcessMatCameraCalibrationMessage(const u8* buffer) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessHeadCameraCalibrationMessage(const u8* buffer) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }
      
      
      
#pragma mark --- VisionSystem::Mailbox Template Implementations ---
      
      bool CheckMailbox(BlockMarkerObserved& msg)
      {
        return blockMarkerMailbox_.getMessage(msg);
      }
      
      bool CheckMailbox(MatMarkerObserved&   msg)
      {
        return matMarkerMailbox_.getMessage(msg);
      }
      
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
