extern "C" {
#include "client.h"
}
#include "messages.h"
#include "animationController.h"
#include "rtip.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init()
      {
        return RESULT_OK;
      }
      
      void Process_abortAnimation(const RobotInterface::AbortAnimation& msg)
      {
        AnimationController::Clear();
      }
      
      void Process_disableAnimTracks(const AnimKeyFrame::DisableAnimTracks& msg)
      {
        AnimationController::DisableTracks(msg.whichTracks);
      }
      
      void Process_enableAnimTracks(const AnimKeyFrame::EnableAnimTracks& msg)
      {
        AnimationController::EnableTracks(msg.whichTracks);
      }

      // Group processor for all animation key frame messages
      void Process_anim(const RobotInterface::EngineToRobot& msg)
      {
        if(AnimationController::BufferKeyFrame(msg) != RESULT_OK) {
          //PRINT("Failed to buffer a keyframe! Clearing Animation buffer!\n");
          AnimationController::Clear();
        }
      }
      
      void ProcessMessage(u8* buffer, u16 bufferSize)
      {
        RobotInterface::EngineToRobot msg;
        if (bufferSize > msg.MAX_SIZE)
        {
          PRINT("Received message too big! %02x[%d]\n", buffer[0], bufferSize);
          return;
        }
        else
        {
          memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
          if (!msg.IsValid())
          {
            PRINT("Received invalid message: %02x[%d]\n", buffer[0], bufferSize);
          }
          else if (msg.tag < 0x80) // Message for RTIP not us
          {
            RTIP::SendMessage(msg);
          }
          else
          {
            switch(msg.tag)
            {
              case 0x90:
              {
                Process_abortAnimation(msg.abortAnimation);
                break;
              }
              case 0x91:
              case 0x92:
              case 0x93:
              case 0x94:
              case 0x95:
              case 0x96:
              case 0x97:
              case 0x98:
              case 0x99:
              case 0x9A:
              case 0x9B:
              {
                Process_anim(msg);
                break;
              }
              case 0x9D:
              {
                Process_disableAnimTracks(msg.disableAnimTracks);
                break;
              }
              case 0x9E:
              {
                Process_enableAnimTracks(msg.enableAnimTracks);
                break;
              }
              default:
              {
                PRINT("Received message not expected here tag=%02x\n", msg.tag);
              }
            }
          }
        }
      } 


      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
        SendText(format, argptr);
        va_end(argptr);
        return 0;
      }

      int SendText(const char *format, va_list vaList)
      {
        RobotInterface::PrintText m;
        int len;
                        
        #define MAX_SEND_TEXT_LENGTH 255 // uint_8 definition in messageRobotToEngine.clad
        memset(m.text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        len = vsnprintf(m.text, MAX_SEND_TEXT_LENGTH, format, vaList);
        
        if (len > 0)
        {
          m.text_length = len;
          RobotInterface::SendMessage(m);
        }

        return 0;
      }
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
