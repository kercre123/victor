extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
  #include "clad/types/imageTypes.h"
  #include "clad/robotInterface/messageRobotToEngine.h"
}

namespace Anki {
namespace Cozmo {

void sendImgChunk(ImageChunk& msg, bool eof)
{
  msg.imageEncoding = JPEGMinimizedGray;
  msg.resolution    = QVGA;
  if (eof)
  {
    msg.frameTimeStamp  = system_get_time(); //XXX Need to replace with GetTimeStamp() as soon as we have it
    msg.imageId++;
    msg.imageChunkCount = msg.chunkId + 1;
  }
  clientSendMessage(msg.GetBuffer(), msg.Size(), RobotInterface::RobotToEngine::Tag_image, false, eof);
  msg.chunkId = eof ? 0 : msg.chunkId + 1;
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
  os_memcpy(msg.data + msg.data_length, imgData, len);
  msg.data_length += len;
  if (eof)
  {
    sendImgChunk(msg, true);
  }
}


} // namespace Cozmo
} // namespace Anki
