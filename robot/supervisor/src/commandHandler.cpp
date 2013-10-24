#include "anki/cozmo/messageProtocol.h"

#include "anki/cozmo/robot/commandHandler.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoTypes.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/messaging/robot/utilMessaging.h"

#include <cmath>
#include <cstdio>
#include <string>



namespace Anki {
  namespace Cozmo {
    namespace CommandHandler {
    
      // "Private Member Variables"
      namespace {
        
      }

      
      void ProcessIncomingMessages()
      {
        // Buffer any incoming data from basestation
        HAL::ManageRecvBuffer();
        
        // Process any messages from the basestation
        u8 msgBuffer[255];
        u8 msgSize;
        while( (msgSize = HAL::RecvMessage(msgBuffer)) > 0 )
        {
          CozmoMsg_Command cmd = static_cast<CozmoMsg_Command>(msgBuffer[1]);
          switch(cmd)
          {
            case MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE:
            {
              // TODO: Double-check that size matches expected size?
              
              const CozmoMsg_AbsLocalizationUpdate *msg = reinterpret_cast<const CozmoMsg_AbsLocalizationUpdate*>(msgBuffer);
              
              f32 currentMatX       = msg->xPosition * .001f; // store in meters
              f32 currentMatY       = msg->yPosition * .001f; //     "
              Radians currentMatHeading = msg->headingAngle;
              Localization::SetCurrentMatPose(currentMatX, currentMatY, currentMatHeading);
              
              fprintf(stdout, "Robot received localization update from "
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
              break;
            }
            default:
              fprintf(stdout, "Unrecognized command in received message.\n");
              
          } // switch(cmd)
        }
        
      }
      
    } // namespace CommandHandler
  } // namespace Cozmo
} // namespace Anki
