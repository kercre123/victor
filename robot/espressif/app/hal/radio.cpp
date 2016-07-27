extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
}
#include "anki/cozmo/robot/hal.h"
#include "rtip.h"
#include "clad/types/imageTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

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
          if ((realID == RobotInterface::RobotToEngine::Tag_trace) || (realID == RobotInterface::RobotToEngine::Tag_printText))
          {
            if (clientQueueAvailable() < 200) return false;
          }
          return clientSendMessage((u8*)buffer, size, msgID, msgID < RobotInterface::TO_ENG_UNREL, false);
        }
      }
    }
    
    void sendImgChunk(ImageChunk& msg, bool eof)
    {
      static bool skipFrame = true; // Want to wait for the first whole frame before sending anything
      msg.imageEncoding = JPEGMinimizedGray;
      msg.resolution    = QVGA;
      
      // As soon as we skip one chunk of a frame, skip the remaining chunks - the robot can't recover it anyway
      if (!skipFrame)
        if (!clientSendMessage(msg.GetBuffer(), msg.Size(), RobotInterface::RobotToEngine::Tag_image, false, eof))
          skipFrame = true;
        
      if (eof)
      {
        msg.chunkId = 0;
        skipFrame = false;
      }
      else
      {
        msg.chunkId++;
      }
      msg.data_length = 0;
    }

    extern "C" void imageSenderQueueData(uint8_t* imgData, uint8_t len, bool eof)
    {
      static ImageChunk msg; // Relying on 0 initalization of static structures
      msg.chunkDebug = len | (eof << 8);
      if (msg.data_length + len > IMAGE_CHUNK_SIZE) // No room left in this chunk for this data
      {
        sendImgChunk(msg, false); // Send what we have
      }
      if (eof)
      {
        os_memcpy(msg.data + msg.data_length, imgData, len-8);
        msg.data_length += len - 8;
        msg.frameTimeStamp = *((u32*)(imgData + len-8));
        msg.imageChunkCount = msg.chunkId + 1;
        sendImgChunk(msg, true);
        msg.frameTimeStamp = 0; // Mark as invalid
        msg.imageId = *((u32*)(imgData + len-4)) + 1; // Set for next frame
        msg.imageChunkCount = 0; // Mark as invalid
      }
      else
      {
        os_memcpy(msg.data + msg.data_length, imgData, len);
        msg.data_length += len;
      }
    }

} // namespace Cozmo
} // namespace Anki
