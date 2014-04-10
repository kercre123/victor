#include "anki/common/robot/trig_fast.h"
#include "imuFilter.h"
#include "headController.h"
#include "anki/cozmo/robot/hal.h"


namespace Anki {
  namespace Cozmo {
    namespace IMUFilter {
      
      namespace {
        
        // Orientation and speed in XY-plane (i.e. horizontal plane) of robot
        f32 rot_ = 0;   // radians
        f32 rotSpeed_ = 0; // rad/s
        
        // Angle of accelerometer wrt gravity horizontal
        f32 pitch_ = 0;
        
        f32 gyro_filt[3];
        f32 gyro_robot_frame_filt[3];
        const f32 RATE_FILT_COEFF = 0.9f;

        f32 accel_filt[3];
        f32 accel_robot_frame_filt[3];
        const f32 ACCEL_FILT_COEFF = 0.4f;
        
      } // "private" namespace
      
      
      void Reset()
      {
        rot_ = 0;
        rotSpeed_ = 0;
      }
      
      ReturnCode Update()
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        // Get IMU data
        HAL::IMU_DataStructure imu_data;
        HAL::IMUReadData(imu_data);
        
        
        ////// Gyro Update //////
        
        // Filter rotation speeds
        // TODO: Do this in hardware?
        gyro_filt[0] = imu_data.rate_x * RATE_FILT_COEFF + gyro_filt[0] * (1.f-RATE_FILT_COEFF);
        gyro_filt[1] = imu_data.rate_y * RATE_FILT_COEFF + gyro_filt[1] * (1.f-RATE_FILT_COEFF);
        gyro_filt[2] = imu_data.rate_z * RATE_FILT_COEFF + gyro_filt[2] * (1.f-RATE_FILT_COEFF);
      
        
        // Compute head angle wrt to world horizontal plane
        f32 headAngle = HeadController::GetAngleRad();  // TODO: Use encoders or accelerometer data? If encoders,
                                                        // may need to use accelerometer data anyway for when it's on ramps.

        

        // Compute rotation speeds in robot XY-plane.
        // https://www.chrobotics.com/library/understanding-euler-angles
        // http://ocw.mit.edu/courses/mechanical-engineering/2-017j-design-of-electromechanical-robotic-systems-fall-2009/course-text/MIT2_017JF09_ch09.pdf
        //
        // r: roll angle (x-axis), p: pitch angle (y-axis), y: yaw angle (z-axis)
        //
        //            |  1    sin(r)*tan(p)    cos(r)*tan(p)  |
        // D(r,p,y) = |  0       cos(r)           -sin(r)     |
        //            |  0    sin(r)/cos(p)    cos(r)/cos(p)  |
        //
        // Rotation in robot frame = D * [dr/dt, dp/dt, dy,dt] where the latter vector is given by gyro readings.
        // In our case, we only care about yaw. In other words, it's always true that r = y = 0.
        // (NOTE: This is true as long as we don't start turning on ramps!!!)
        // So the result simplifies to...
        gyro_robot_frame_filt[0] = gyro_filt[0] + gyro_filt[2] * tanf(headAngle);
        gyro_robot_frame_filt[1] = gyro_filt[1];
        gyro_robot_frame_filt[2] = gyro_filt[2] / cosf(headAngle);
        // TODO: We actually only care about gyro_robot_frame_filt[2]. Any point in computing the others?
        
        
        // XY-plane rotation rate is robot frame z-axis rotation rate
        rotSpeed_ = gyro_robot_frame_filt[2];
        
        
        // Update orientation
        f32 dAngle = rotSpeed_ * CONTROL_DT;
        rot_ += dAngle;
        
        
        
        ///// Accelerometer update /////
        accel_filt[0] = imu_data.acc_x * ACCEL_FILT_COEFF + accel_filt[0] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[1] = imu_data.acc_y * ACCEL_FILT_COEFF + accel_filt[1] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[2] = imu_data.acc_z * ACCEL_FILT_COEFF + accel_filt[2] * (1.f-ACCEL_FILT_COEFF);
        
        pitch_ = atan2(accel_filt[0], accel_filt[2]);
        
        //PERIODIC_PRINT(50, "Pitch %f\n", RAD_TO_DEG_F32(pitch_));
        
        return retVal;
        
      } // Update()
      
      
      f32 GetRotation()
      {
        return rot_;
      }
      
      f32 GetRotationSpeed()
      {
        return rotSpeed_;
      }
      
      f32 GetPitch()
      {
        return pitch_;
      }
      

    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki
