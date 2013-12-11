#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

/*
 class VisionSystem
 {
 public:
 */

#include "anki/common/types.h"

#include "anki/common/robot/geometry_declarations.h"

#include "anki/cozmo/MessageProtocol.h"

#include "anki/cozmo/robot/hal.h"

// If enabled, will use Matlab as the vision system for processing images
#if defined(SIMULATOR) && ANKICORETECH_USE_MATLAB
#define USE_MATLAB_FOR_HEAD_CAMERA
#define USE_MATLAB_FOR_MAT_CAMERA
#endif

namespace Anki {
  namespace Cozmo {
   
    namespace VisionSystem {

      //
      // Typedefs
      //
      
     
      //
      // Parameters / Constants:
      //
      
      const HAL::CameraMode DETECTION_RESOLUTION = HAL::CAMERA_MODE_QVGA;
      const HAL::CameraMode TRACKING_RESOLUTION  = HAL::CAMERA_MODE_QQQVGA;
      
      const HAL::CameraMode MAT_LOCALIZATION_RESOLUTION = HAL::CAMERA_MODE_QVGA;
      const HAL::CameraMode MAT_ODOMETRY_RESOLUTION     = HAL::CAMERA_MODE_QQQQVGA;
      
      const u8 MAX_BLOCK_MARKER_MESSAGES = 32; // max blocks we can see in one image
      const u8 MAX_MAT_MARKER_MESSAGES   = 1;
      const u8 MAX_DOCKING_MESSAGES      = 1;
      
      
      //
      // Mailboxes
      //
      //   VisionSystem "Mailboxes" are used for leaving messages from slower
      //   vision processing in LongExecution to be retrieved and acted upon
      //   by the faster MainExecution.
      //
      
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
      typedef Mailbox<CozmoMsg_BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> BlockMarkerMailbox;
      
      typedef Mailbox<CozmoMsg_MatMarkerObserved, MAX_MAT_MARKER_MESSAGES> MatMarkerMailbox;
      
      typedef Mailbox<CozmoMsg_DockingErrorSignal, MAX_DOCKING_MESSAGES> DockingMailbox;
  
      
      //
      // Methods
      //
      
      ReturnCode Init(BlockMarkerMailbox*     blockMarkerMailbox,
                      MatMarkerMailbox*       matMarkerMailbox,
                      DockingMailbox*         dockingMailbox);
      
      void Destroy();
      
      ReturnCode Update(u8* memoryBuffer);

      // Select a block type to look for to dock with.  Use 0 to disable.
      ReturnCode SetDockingBlock(const u16 blockTypeToDockWith = 0);
            
      bool IsInitialized();
      
    } // namespace VisionSystem
    
    
    #pragma mark --- VisionSystem::Mailbox Template Implementations ---
    
    template<typename MsgType, u8 NumMessages>
    VisionSystem::Mailbox<MsgType,NumMessages>::Mailbox() : isLocked(false)
    {
      for(u8 i=0; i<NumMessages; ++i) {
        this->beenRead[i] = true;
      }
    }
    
    template<typename MsgType, u8 NumMessages>
    void VisionSystem::Mailbox<MsgType,NumMessages>::lock()
    {
      this->isLocked = true;
    }
    
    template<typename MsgType, u8 NumMessages>
    void VisionSystem::Mailbox<MsgType,NumMessages>::unlock()
    {
      this->isLocked = false;
    }
    
    template<typename MsgType, u8 NumMessages>
    bool VisionSystem::Mailbox<MsgType,NumMessages>::hasMail() const
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
    void VisionSystem::Mailbox<MsgType,NumMessages>::putMessage(const MsgType newMsg)
    {
      messages[writeIndex] = newMsg;
      beenRead[writeIndex] = false;
      advanceIndex(writeIndex);
    }
    
    template<typename MsgType, u8 NumMessages>
    MsgType VisionSystem::Mailbox<MsgType,NumMessages>::getMessage()
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
    void VisionSystem::Mailbox<MsgType,NumMessages>::advanceIndex(u8 &index)
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
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
