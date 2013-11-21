#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

/*
 class VisionSystem
 {
 public:
 */

#include "anki/common/types.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/MessageProtocol.h"


// If enabled, will use Matlab as the vision system for processing images
#ifdef SIMULATOR
#define USE_MATLAB_FOR_HEAD_CAMERA
#define USE_MATLAB_FOR_MAT_CAMERA
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace VisionSystem {
      
      const u8 MAX_BLOCK_MARKER_MESSAGES = 32;
      const u8 MAX_MAT_MARKER_MESSAGES   = 1;
      
      template<typename MsgType, u8 NumMessages>
      class Mailbox
      {
      public:
        Mailbox();
        
        bool hasMail(void) const;
        void putMessage(const MsgType newMsg);
        MsgType getMessage();
        
      protected:
        MsgType messages[NumMessages];
        bool    beenRead[NumMessages];
        u8 readIndex, writeIndex;
        
        void advanceIndex(u8 &index);
      };
      
      typedef struct {
        f32 dotX[4];
        f32 dotY[4];
      } DockingTarget;
      
      typedef Mailbox<CozmoMsg_ObservedBlockMarker, MAX_BLOCK_MARKER_MESSAGES> BlockMarkerMailbox;
      
      typedef Mailbox<CozmoMsg_ObservedMatMarker, MAX_MAT_MARKER_MESSAGES> MatMarkerMailbox;
      
      
      ReturnCode Init(const HAL::CameraInfo*  headCamInfo,
                      const HAL::CameraInfo*  matCamInfo,
                      BlockMarkerMailbox*     blockMarkerMailbox,
                      MatMarkerMailbox*       matMarkerMailbox);
      
      void Destroy();
      
      ReturnCode lookForBlocks();
      ReturnCode findDockingTarget(DockingTarget& target);
      ReturnCode localizeWithMat();
      ReturnCode setDockingWindow(const s16 xLeft, const s16 yTop,
                                  const s16 width, const s16 height);
      
      bool IsInitialized();
      
    } // namespace VisionSystem
    
    
    #pragma mark --- VisionSystem::Mailbox Template Implementations ---
    
    template<typename MsgType, u8 NumMessages>
    VisionSystem::Mailbox<MsgType,NumMessages>::Mailbox()
    {
      for(u8 i=0; i<NumMessages; ++i) {
        this->beenRead[i] = false;
      }
    }
    
    template<typename MsgType, u8 NumMessages>
    bool VisionSystem::Mailbox<MsgType,NumMessages>::hasMail() const
    {
      if(not beenRead[readIndex]) {
        return true;
      } else {
        return false;
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
      u8 toReturn = readIndex;
      
      advanceIndex(readIndex);
      
      this->beenRead[toReturn] = true;
      return this->messages[toReturn];
    }
    
    template<typename MsgType, u8 NumMessages>
    void VisionSystem::Mailbox<MsgType,NumMessages>::advanceIndex(u8 &index)
    {
      ++index;
      if(index == NumMessages) {
        index = 0;
      }
    }
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
