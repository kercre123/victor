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
        const u8 MAX_MAT_MARKER_MESSAGES   = 1;
        const u8 MAX_DOCKING_MESSAGES      = 1;
        
        template<typename MsgType, u8 NumMessages>
        class Mailbox
        {
        public:
          Mailbox();
          
          // True if there are unread messages left in the mailbox
          bool hasMail(void) const;
          
          // Add a message to the mailbox
          void putMessage(const MsgType newMsg);
          
          // Take a message out of the mailbox
          MsgType getMessage();
          
        protected:
          MsgType messages[NumMessages];
          bool    beenRead[NumMessages];
          u8 readIndex, writeIndex;
          bool isLocked;
          
          void lock();
          void unlock();
          void advanceIndex(u8 &index);
        };
        
        // Typedefs for mailboxes to hold different types of messages:
        typedef Mailbox<Messages::BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> BlockMarkerMailbox;
        
        typedef Mailbox<Messages::MatMarkerObserved, MAX_MAT_MARKER_MESSAGES> MatMarkerMailbox;
        
        typedef Mailbox<Messages::DockingErrorSignal, MAX_DOCKING_MESSAGES> DockingMailbox;
        
        BlockMarkerMailbox blockMarkerMailbox_;
        MatMarkerMailbox   matMarkerMailbox_;
        DockingMailbox     dockingMailbox_;
        
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
        bool retVal = false;
        if(blockMarkerMailbox_.hasMail()) {
          retVal = true;
          msg = blockMarkerMailbox_.getMessage();
        }
        
        return retVal;
      }
      
      bool CheckMailbox(MatMarkerObserved&   msg)
      {
        bool retVal = false;
        if(matMarkerMailbox_.hasMail()) {
          retVal = true;
          msg = matMarkerMailbox_.getMessage();
        }
        
        return retVal;
      }
      
      bool CheckMailbox(DockingErrorSignal&  msg)
      {
        bool retVal = false;
        if(dockingMailbox_.hasMail()) {
          retVal = true;
          msg = dockingMailbox_.getMessage();
        }
        
        return retVal;
      }
      
      
      template<typename MsgType, u8 NumMessages>
      Mailbox<MsgType,NumMessages>::Mailbox() : isLocked(false)
      {
        for(u8 i=0; i<NumMessages; ++i) {
          this->beenRead[i] = true;
        }
      }
      
      template<typename MsgType, u8 NumMessages>
      void Mailbox<MsgType,NumMessages>::lock()
      {
        this->isLocked = true;
      }
      
      template<typename MsgType, u8 NumMessages>
      void Mailbox<MsgType,NumMessages>::unlock()
      {
        this->isLocked = false;
      }
      
      template<typename MsgType, u8 NumMessages>
      bool Mailbox<MsgType,NumMessages>::hasMail() const
      {
        if(isLocked) {
          return false;
        }
        else {
          if(not beenRead[readIndex]) {
            return true;
          } else {
            return false;
          }
        }
      }
      
      template<typename MsgType, u8 NumMessages>
      void Mailbox<MsgType,NumMessages>::putMessage(const MsgType newMsg)
      {
        messages[writeIndex] = newMsg;
        beenRead[writeIndex] = false;
        advanceIndex(writeIndex);
      }
      
      template<typename MsgType, u8 NumMessages>
      MsgType Mailbox<MsgType,NumMessages>::getMessage()
      {
        if(this->isLocked) {
          // Is this the right thing to do if locked?
          return this->messages[readIndex];
        }
        else {
          u8 toReturn = readIndex;
          
          advanceIndex(readIndex);
          
          this->beenRead[toReturn] = true;
          return this->messages[toReturn];
        }
      }
      
      template<typename MsgType, u8 NumMessages>
      void Mailbox<MsgType,NumMessages>::advanceIndex(u8 &index)
      {
        if(NumMessages==1) {
          // special case
          index = 0;
        }
        else {
          ++index;
          if(index == NumMessages) {
            index = 0;
          }
        }
      }
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
