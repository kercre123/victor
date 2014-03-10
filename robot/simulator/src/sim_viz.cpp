#include <cstdlib>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/VizStructs.h"
#include "sim_viz.h"

#include <webots/Supervisor.hpp>

namespace Anki {
  namespace Cozmo {
    
    namespace Sim {
      extern webots::Supervisor* CozmoBot;

      namespace Viz {
        
        namespace {
          // For communications with the cozmo_physics plugin used for drawing
          // paths with OpenGL.  Note that plugin comms uses channel 0.
          webots::Emitter *physicsComms_ = Sim::CozmoBot->getEmitter("cozmo_physics_comms");
          
          static const u32 MAX_SIZE_SEND_BUF = 128;
          char sendBuf[MAX_SIZE_SEND_BUF];
        }
        
        void ErasePath(s32 path_id)
        {
          VizErasePath msg;
          msg.pathID = path_id;
          
          sendBuf[0] = VizErasePath_ID;
          memcpy(sendBuf + 1, &msg, sizeof(msg));
          physicsComms_->send(sendBuf, sizeof(msg)+1);
          
          //PRINT("ERASE PATH: %d bytes\n", (int)sizeof(msg) + 1);

        }
        
        void AppendPathSegmentLine(s32 path_id, f32 x_start_mm, f32 y_start_mm, f32 x_end_mm, f32 y_end_mm)
        {
          VizAppendPathSegmentLine msg;
          msg.pathID = path_id;
          msg.x_start_m = MM_TO_M(x_start_mm);
          msg.y_start_m = MM_TO_M(y_start_mm);
          msg.z_start_m = 0;
          msg.x_end_m = MM_TO_M(x_end_mm);
          msg.y_end_m = MM_TO_M(y_end_mm);
          msg.z_end_m = 0;
          
          sendBuf[0] = VizAppendPathSegmentLine_ID;
          memcpy(sendBuf + 1, &msg, sizeof(msg));
          physicsComms_->send(sendBuf, sizeof(msg)+1);
          
          //PRINT("APPEND LINE: %d bytes\n", (int)sizeof(msg) + 1);
        }
        
        void AppendPathSegmentArc(s32 path_id, f32 x_center_mm, f32 y_center_mm, f32 radius_mm, f32 startRad, f32 sweepRad)
        {
          VizAppendPathSegmentArc msg;
          msg.pathID = path_id;
          msg.x_center_m = MM_TO_M(x_center_mm);
          msg.y_center_m = MM_TO_M(y_center_mm);
          msg.radius_m = MM_TO_M(radius_mm);
          msg.start_rad = startRad;
          msg.sweep_rad = sweepRad;
          
          sendBuf[0] = VizAppendPathSegmentArc_ID;
          memcpy(sendBuf + 1, &msg, sizeof(msg));
          physicsComms_->send(sendBuf, sizeof(msg)+1);
          
          //PRINT("APPEND ARC: %d bytes\n", (int)sizeof(msg) + 1);
        }
        
      } // namespace Viz
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki