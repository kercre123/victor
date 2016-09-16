extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
  #include "imageSender.h"
}
#include "anki/cozmo/robot/hal.h"
#include "rtip.h"
#include "clad/types/imageTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

// Confirm pure C def agrees with clad
ct_assert(IMAGE_DATA_MAX_LENGTH == Anki::Cozmo::IMAGE_CHUNK_SIZE);
// We need the purce C ImageDataBuffer and CLAD Anki::Cozmo::ImageChunk to be binary compatible
ct_assert(sizeof(ImageDataBuffer) == sizeof(Anki::Cozmo::ImageChunk));

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
      {
        const u8 realID = (msgID == RobotInterface::GLOBAL_INVALID_TAG) ? *((u8*)buffer) : msgID;
        if (realID < RobotInterface::TO_WIFI_START)
        {
          return RTIP::SendMessage((const u8*)buffer, size, msgID);
        }
        else
        {
          // Check if this is a trace or text message (debugging)
          if ((realID == RobotInterface::RobotToEngine::Tag_trace) ||
              (realID == RobotInterface::RobotToEngine::Tag_printText))
          {
            if (clientQueueAvailable() < LOW_PRIORITY_BUFFER_ROOM) return false;
          }
          return clientSendMessage((u8*)buffer, size, msgID, realID < RobotInterface::TO_ENG_UNREL, false);
        }
      }
      
      bool RadioIsConnected() { return clientConnected(); }
      
      int RadioQueueAvailable() { return clientQueueAvailable(); }
    }

    /** Task for sending image data
     */
    extern "C" void sendImageTask(os_event_t *event)
    {
      static u32  imageId = 0;
      static u8   chunkId = 0;
      static bool skipFrame = false;
      const uint32_t flags = event->sig;
      
      if (unlikely(flags & QIS_overflow))
      {
        skipFrame = true;
        chunkId = 0;
        imageId = 0;
      }
      else if (unlikely(flags & QIS_resume))
      {
        skipFrame = false;
        imageId = event->par + 1;
        chunkId = 0;
      }
      else
      {
        ImageChunk* msg = (ImageChunk*)(event->par);
        const bool eof = flags & QIS_endOfFrame;
        const u32* meta = (u32*)(msg->data + msg->data_length - 8);

        msg->status++; // Indicate the task is processing the message
        msg->imageEncoding = JPEGMinimizedGray;
        msg->resolution    = QVGA;
        msg->imageId       = imageId;
        msg->chunkId       = chunkId;
        
        if (skipFrame == false)
        {
          if (eof)
          {
            msg->data_length -= 8;
            msg->frameTimeStamp = meta[0];
            msg->imageChunkCount = chunkId + 1;
          }
          if (!clientSendMessage(msg->GetBuffer(), msg->Size(), RobotInterface::RobotToEngine::Tag_image, false, eof))
          {
            skipFrame = true;
          }
        }
        
        if (eof)
        {
          imageId = meta[1] + 1; // Set for next frame
          chunkId = 0;
          skipFrame = false;
        }
        else
        {
          chunkId++;
        }
        
        msg->imageChunkCount = 0; // Mark as invalid
        msg->frameTimeStamp = 0; // Mark as invalid
        msg->data_length = 0;
        msg->status = 0; // Mark as transmitted
      }
    }

} // namespace Cozmo
} // namespace Anki
