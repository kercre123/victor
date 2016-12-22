//
// Dedicated file for Messages::CompressAndSendImage
//
// Because CompressAndSendImage() uses array2d, which uses anki/common/robot/errorHandling.h which contains definitions of
// Anki* logging messages, it doesn't play well with anki/cozmo/robot/logging.h which contains duplicate definitions.
// So this one function using array2d was pulled out here so that we can still use the newer logging.h in other files.
//
// NOTE: PRINT doesn't actually do anything from here.
//

#ifdef SIMULATOR

#include "messages.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#endif


namespace Anki {
  namespace Cozmo {
    namespace Messages {

      namespace {
        const int IMAGE_SEND_JPEG_COMPRESSION_QUALITY = 50; // 0 to 100
      } // private namespace

// #pragma mark --- Messages Method Implementations ---

      Result CompressAndSendImage(const u8* img, const s32 captureHeight, const s32 captureWidth, const TimeStamp_t captureTime)
      {
        ImageChunk m;
        
        switch(captureHeight) {
          case 240:
            if (captureWidth!=320*3) {
              PRINT("CompressAndSendImage - Unrecognized resolution: %dx%d.\n", captureWidth/3, captureHeight);
              return RESULT_FAIL;
            }
            m.resolution = QVGA;
            break;

          case 296:
            if (captureWidth!=400*3) {
              PRINT("CompressAndSendImage - Unrecognized resolution: %dx%d.\n", captureWidth/3, captureHeight);
              return RESULT_FAIL;
            }
            m.resolution = CVGA;
            break;

          case 480:
            if (captureWidth!=640*3) {
              PRINT("CompressAndSendImage - Unrecognized resolution: %dx%d.\n", captureWidth/3, captureHeight);
              return RESULT_FAIL;
            }
            m.resolution = VGA;
            break;

          default:
            PRINT("CompressAndSendImage - Unrecognized resolution: %dx%d.\n", captureWidth/3, captureHeight);
            return RESULT_FAIL;
        }

        static u32 imgID = 0;
        const std::vector<int> compressionParams = {
          CV_IMWRITE_JPEG_QUALITY, IMAGE_SEND_JPEG_COMPRESSION_QUALITY
        };

        cv::Mat cvImg;
        cvImg = cv::Mat(captureHeight, captureWidth/3, CV_8UC3, (void*)img);
        cvtColor(cvImg, cvImg, CV_BGR2RGB);

        std::vector<u8> compressedBuffer;
        cv::imencode(".jpg",  cvImg, compressedBuffer, compressionParams);

        const u32 numTotalBytes = static_cast<u32>(compressedBuffer.size());

        //PRINT("Sending frame with capture time = %d at time = %d\n", captureTime, HAL::GetTimeStamp());

        m.frameTimeStamp = captureTime;
        m.imageId = ++imgID;
        m.chunkId = 0;
        m.data_length = IMAGE_CHUNK_SIZE;
        m.imageChunkCount = ceilf((f32)numTotalBytes / IMAGE_CHUNK_SIZE);
        m.imageEncoding = JPEGColor;

        u32 totalByteCnt = 0;
        u32 chunkByteCnt = 0;

        for(s32 i=0; i<numTotalBytes; ++i)
        {
          m.data[chunkByteCnt] = compressedBuffer[i];

          ++chunkByteCnt;
          ++totalByteCnt;

          if (chunkByteCnt == IMAGE_CHUNK_SIZE) {
            //PRINT("Sending image chunk %d\n", m.chunkId);
            RobotInterface::SendMessage(m);
            ++m.chunkId;
            chunkByteCnt = 0;
          } else if (totalByteCnt == numTotalBytes) {
            // This should be the last message!
            //PRINT("Sending LAST image chunk %d\n", m.chunkId);
            m.data_length = chunkByteCnt;
            RobotInterface::SendMessage(m);
          }
        } // for each byte in the compressed buffer

        return RESULT_OK;
      } // CompressAndSendImage()

    } // namespace Messages

  }
}

#endif // SIMULATOR