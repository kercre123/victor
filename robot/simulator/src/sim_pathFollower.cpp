#include <cstdlib>

#include "anki/cozmo/robot/hal.h"

#include "sim_pathFollower.h"

#include "cozmo_physics.h"

#include <webots/Supervisor.hpp>

namespace Anki {
  namespace Cozmo {
    
    namespace Sim {
      extern webots::Supervisor* CozmoBot;
    }
    
    namespace PathFollower {
      namespace Viz {
        
        namespace {
          // For communications with the cozmo_physics plugin used for drawing
          // paths with OpenGL.  Note that plugin comms uses channel 0.
          webots::Emitter *physicsComms_ = Sim::CozmoBot->getEmitter("cozmo_physics_comms");
        }
        
        ReturnCode Init(void)
        {
          
          // Initialize path drawing settings
          SetPathHeightOffset(0.05);
          
          return EXIT_SUCCESS;
          
        } // Init()
        
        void ErasePath(s32 path_id)
        {
          f32 msg[ERASE_PATH_MSG_SIZE];
          msg[0] = PLUGIN_MSG_ERASE_PATH;
          msg[PLUGIN_MSG_ROBOT_ID] = HAL::GetRobotID();
          msg[PLUGIN_MSG_PATH_ID] = path_id;
          physicsComms_->send(msg, sizeof(msg));
        }
        
        void AppendPathSegmentLine(s32 path_id, f32 x_start_mm, f32 y_start_mm, f32 x_end_mm, f32 y_end_mm)
        {
          f32 msg[LINE_MSG_SIZE];
          msg[0] = PLUGIN_MSG_APPEND_LINE;
          msg[PLUGIN_MSG_ROBOT_ID] = HAL::GetRobotID();
          msg[PLUGIN_MSG_PATH_ID] = path_id;
          msg[LINE_START_X] = x_start_mm * 0.001;
          msg[LINE_START_Y] = y_start_mm * 0.001;
          msg[LINE_END_X] = x_end_mm * 0.001;
          msg[LINE_END_Y] = y_end_mm * 0.001;
          
          physicsComms_->send(msg, sizeof(msg));
        }
        
        void AppendPathSegmentArc(s32 path_id, f32 x_center_mm, f32 y_center_mm, f32 radius_mm, f32 startRad, f32 sweepRad)
        {
          f32 msg[ARC_MSG_SIZE];
          msg[0] = PLUGIN_MSG_APPEND_ARC;
          msg[PLUGIN_MSG_ROBOT_ID] = HAL::GetRobotID();
          msg[PLUGIN_MSG_PATH_ID] = path_id;
          msg[ARC_CENTER_X] = x_center_mm * 0.001;
          msg[ARC_CENTER_Y] = y_center_mm * 0.001;
          msg[ARC_RADIUS] = radius_mm * 0.001;
          msg[ARC_START_RAD] = startRad;
          msg[ARC_SWEEP_RAD] = sweepRad;
          
          physicsComms_->send(msg, sizeof(msg));
        }
        
        void ShowPath(s32 path_id, bool show)
        {
          f32 msg[SHOW_PATH_MSG_SIZE];
          msg[0] = PLUGIN_MSG_SHOW_PATH;
          msg[PLUGIN_MSG_ROBOT_ID] = HAL::GetRobotID();
          msg[PLUGIN_MSG_PATH_ID] = path_id;
          msg[SHOW_PATH] = show ? 1 : 0;
          physicsComms_->send(msg, sizeof(msg));
        }
        
        void SetPathHeightOffset(f32 m){
          f32 msg[SET_HEIGHT_OFFSET_MSG_SIZE];
          msg[0] = PLUGIN_MSG_SET_HEIGHT_OFFSET;
          msg[PLUGIN_MSG_ROBOT_ID] = HAL::GetRobotID();
          //msg[PLUGIN_MSG_PATH_ID] = path_id;
          msg[HEIGHT_OFFSET] = m;
          physicsComms_->send(msg, sizeof(msg));
        }
        
        
      } // namespace Viz
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki