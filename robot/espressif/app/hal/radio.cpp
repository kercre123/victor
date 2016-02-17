extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
}
#include "anki/cozmo/robot/hal.h"
#include "clad/types/imageTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
      {
        return clientSendMessage((u8*)buffer, size, msgID);
      }
    }
    
    static u32 imageChunkTimestamp_;
    
    void sendImgChunk(ImageChunk& msg, bool eof)
    {
      msg.imageEncoding = JPEGMinimizedGray;
      msg.resolution    = QVGA;
      if (eof)
      {
        msg.frameTimeStamp  = imageChunkTimestamp_;
        msg.imageChunkCount = msg.chunkId + 1;
      }
      clientSendMessage(msg.GetBuffer(), msg.Size(), RobotInterface::RobotToEngine::Tag_image);
      if (eof)
      {
        msg.imageId++;
        msg.chunkId = 0;
      }
      else
      {
        msg.chunkId++;
      }
      msg.imageChunkCount = 0;
      msg.data_length = 0;
    }

    extern "C" void imageSenderQueueData(uint8_t* imgData, uint8_t len, bool eof)
    {
      static ImageChunk msg; // Relying on 0 initalization of static structures
      if (msg.data_length + len > IMAGE_CHUNK_SIZE) // No room left in this chunk for this data
      {
        sendImgChunk(msg, false); // Send what we have
      }
      if (eof)
      {
        os_memcpy(msg.data + msg.data_length, imgData, len-4);
        msg.data_length += len - 4;
        imageChunkTimestamp_ = *((u32*)(imgData + len-4));
        sendImgChunk(msg, true);
      }
      else
      {
        os_memcpy(msg.data + msg.data_length, imgData, len);
        msg.data_length += len;
      }
    }

} // namespace Cozmo
} // namespace Anki
