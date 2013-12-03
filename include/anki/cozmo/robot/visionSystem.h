#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

/*
 class VisionSystem
 {
 public:
 */

#include "anki/common/types.h"

#include "anki/common/robot/geometry_declarations.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/MessageProtocol.h"


// If enabled, will use Matlab as the vision system for processing images
#if defined(SIMULATOR) && ANKICORETECH_USE_MATLAB
#define USE_MATLAB_FOR_HEAD_CAMERA
#define USE_MATLAB_FOR_MAT_CAMERA
#endif

// If enabled, frames will be sent out over the serial line for processing
// by Matlab.
#define SERIAL_IMAGING

namespace Anki {
  namespace Cozmo {
    
    namespace VisionSystem {
      
      typedef struct {
        u16 width;
        u16 height;
      } ImageSize;
    
      
      const u8 MAX_BLOCK_MARKER_MESSAGES = 32;
      const u8 MAX_MAT_MARKER_MESSAGES   = 1;
      const u8 MAX_DOCKING_MESSAGES      = 1;
      
      // VisionSystem "Mailboxes" are used for leaving messages from slower
      // vision processing in LongExecution to be retrieved and acted upon
      // by the faster MainExecution.
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
      
      typedef Mailbox<CozmoMsg_DockingErrorSignal, MAX_DOCKING_MESSAGES> DockingMailbox;
      
      
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
      ReturnCode trackDockingTarget();
      
      
      // Visual-servoing Docker "class"
      namespace Docker {
        
        const ImageSize DETECTION_RESOLUTION = {.width = 320, .height = 240};
        const ImageSize TRACKING_RESOLUTION  = {.width =  80, .height =  60};
        
        
        void InitDetection();
        
        
      } // namespace Docker
      
      // LK Tracker "class":
      // TODO: move this generic implementation to coretech-vision?
      namespace LKtracker {
        
        
        // 3x3 linear transformation matrix
        typedef f32[3][3] TForm;
        
        //typedef Embedded::Point<f32> Corner;
        typedef Embedded::Quadrilateral<f32> Corners_t;
        
        // Start a new tracker with an image and the corners of the template in
        // that image to be tracked -- both at DETECTION_RESOLUTION.
        void Init(const u8* img, const Corners &corners);
        
        // Track current template into the next image.  This will update
        // the current transform and corner locations
        bool Track(const u8* img);
        
        // Get the current transformation matrix
        const TForm& GetTransform();
        
        // Get the current corner locations (at TRACKING_RESOLUTION)
        const Corner& GetCorner_UpperLeft();
        const Corner& GetCorner_LowerLeft();
        const Corner& GetCorner_UpperRight();
        const Corner& GetCorner_LowerRight();
        
      } // namespace LKtracker
      
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
