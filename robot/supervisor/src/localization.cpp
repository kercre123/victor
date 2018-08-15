#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "localization.h"
#include "pickAndPlaceController.h"
#include "imuFilter.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include <math.h>

#define DEBUG_LOCALIZATION 0
#define DEBUG_POSE_HISTORY 0

#ifdef SIMULATOR
// Whether or not to use simulator "ground truth" pose
#define USE_SIM_GROUND_TRUTH_POSE 0
#define USE_OVERLAY_DISPLAY 1
#else // else not simulator
#define USE_SIM_GROUND_TRUTH_POSE 0
#define USE_OVERLAY_DISPLAY 0
#endif // #ifdef SIMULATOR


#if(USE_OVERLAY_DISPLAY)
#include "anki/cozmo/robot/hal.h"
#include "simulator/robot/sim_overlayDisplay.h"
#endif

// Slip modelling
// 0 == no slip modelling
// 1 == Assumes slower tread slips
// 2 == Assumes total dist travelled per tic is discounted more the sharper you turn
#define SLIP_MODELLING 2

namespace Anki {
  namespace Vector {
    namespace Localization {

      struct PoseStamp {
        TimeStamp_t t;
        f32 x;
        f32 y;
        f32 angle;
        PoseFrameID_t frame;
        PoseOriginID_t originID;
      };

      namespace {

        const f32 BIG_RADIUS = 5000;

        // private members
        ::Anki::Embedded::Pose2d currMatPose;

        // Localization:
        f32 x_=0.f, y_=0.f;  // mm
        Radians orientation_(0.f);

        // Pose of the robot's drive center which is carry state dependent
        f32 driveCenter_x_ = 0.f, driveCenter_y_ = 0.f;

#if(USE_OVERLAY_DISPLAY)
        f32 xTrue_, yTrue_, angleTrue_;
        f32 prev_xTrue_, prev_yTrue_, prev_angleTrue_;
#endif

        f32 prevLeftWheelPos_ = 0;
        f32 prevRightWheelPos_ = 0;

        f32 gyroRotOffset_ = 0;

        PoseFrameID_t frameId_ = 0;
        PoseOriginID_t originId_ = 0;
        
        // Tread slip modelling
        // Value ranges from 0 to 1.
        // TODO: This value may change for different durometer treads
        f32 slipFactor_ = 1.f;
        


        ///// Pose history ////
        
        // The number of mainExecution tics in between history entries (min: 1)
        const u8 POSE_HISTORY_RES_IN_CYCLES = 6;
        
        // Never need to erase elements, just overwrite with new data.
        const u8 POSE_HISTORY_SIZE = 600/(ROBOT_TIME_STEP_MS * POSE_HISTORY_RES_IN_CYCLES); // 600ms of history

        PoseStamp hist_[POSE_HISTORY_SIZE];
        u16 hStart_ = 0;
        u16 hEnd_ = 0;
        u16 hSize_ = 0;

        // The time of the last keyframe that was used to update the robot's pose.
        // Using this to limit how often keyframes are used to compute the robot's
        // current pose so that we don't have to do multiple
        TimeStamp_t lastKeyframeUpdate_ = 0;
      }



      /// ============= Pose history ==============
      void ClearHistory() {
        hStart_ = 0;
        hEnd_ = 0;
        hSize_ = 0;
        lastKeyframeUpdate_ = 0;
      }


      // Interpolates the pose at time targetTime which should be between historical pose1 time and historical pose2 time.
      // Puts result in history at index poseResult_idx.
      Result InterpolatePose(int pose1_idx, int pose2_idx, f32 targetTime, int poseResult_idx)
      {
        PoseStamp *p1 = &(hist_[pose1_idx]);
        PoseStamp *p2 = &(hist_[pose2_idx]);

        if (p1->t > p2->t) {
          AnkiWarn( "Localization.InterpolatePose.PosesOutOfOrder", "pose2 is older than pose1");
          return RESULT_FAIL;
        }

        if (targetTime < p1->t || targetTime > p2->t) {
          AnkiWarn( "Localization.InterpolatePose.TargetTimeOOR", "targetTime is outside expected range");
          return RESULT_FAIL;
        }

        f32 scale = (targetTime - p1->t) / (p2->t - p1->t);

        f32 x = p1->x + scale * (p2->x - p1->x);
        f32 y = p1->y + scale * (p2->y - p1->y);
        Radians angDiff = Radians(p2->angle) - Radians(p1->angle);
        f32 angle = (Radians(p1->angle) + scale * angDiff).ToFloat();

        PoseStamp *pRes = &(hist_[poseResult_idx]);
        pRes->x = x;
        pRes->y = y;
        pRes->angle = angle;
        pRes->t = targetTime;

        return RESULT_OK;
      }


      Result GetHistIdx(TimeStamp_t t, u16& idx)
      {
        // TODO: Binary search for timestamp
        //       For now just doing a straight up linear search.

        // Check if t is older than oldest pose in history
        // or newer than newest pose in history.
        if (hSize_ < 1 || t < hist_[hStart_].t || t > hist_[hEnd_].t) {
          return RESULT_FAIL;
        }

        u16 prevIdx = hStart_;
        for (idx = hStart_; prevIdx != hEnd_; ++idx) {

          // Check if we hit the end of the history array
          if (idx >= POSE_HISTORY_SIZE) {
            idx = 0;
          }

          if (hist_[idx].t == t) {
            return RESULT_OK;
          } else if (hist_[idx].t > t) {

            // TODO: Does this interpolation really help that much?
            //       Poses in history are already really close together (5ms).
            //       Maybe just pick the closest pose?
            return InterpolatePose(prevIdx, idx, t, idx);
          }
          prevIdx = idx;
        }

        return RESULT_FAIL;
      }

      void AddPoseToHist()
      {
        if (++hEnd_ == POSE_HISTORY_SIZE) {
          hEnd_ = 0;
        }
        
        if (hEnd_ == hStart_) {
          if (++hStart_ == POSE_HISTORY_SIZE) {
            hStart_ = 0;
          }
        } else {
          ++hSize_;
        }
        
        hist_[hEnd_].t = HAL::GetTimeStamp();
        hist_[hEnd_].x = x_;
        hist_[hEnd_].y = y_;
        hist_[hEnd_].angle = orientation_.ToFloat();
        hist_[hEnd_].frame = frameId_;
        hist_[hEnd_].originID = originId_;
      }
      
      
      Result UpdatePoseWithKeyframe(PoseOriginID_t originID,
                                    PoseFrameID_t frameID,
                                    TimeStamp_t t,
                                    const f32 x,
                                    const f32 y,
                                    const f32 angle)
      {
        // Update frameID
        frameId_ = frameID;
        originId_ = originID;
        
        u16 i;
        if (t == 0) {
          // If t==0, this is considered to be a command to just update the current pose
          AnkiInfo( "Localization.UpdatePoseWithKeyFrame.SettingPose", "x= %f, y= %f, angle= %f", x, y, angle);
          SetCurrPose(x, y, angle);
          return RESULT_OK;
        } else if (GetHistIdx(t, i) == RESULT_FAIL) {
          AnkiWarn( "Localization.UpdatePoseWithKeyFrame.TimeNotFound", "Couldn't find timestamp %d in history (oldest(%d) %d, newest(%d) %d)",
                   t, hStart_, hist_[hStart_].t, hEnd_, hist_[hEnd_].t);
          return RESULT_FAIL;
        }

        if (lastKeyframeUpdate_ > t) {
          // We last updated our pose at lastKeyFrameUpdate. Ignore any new information
          // timestamped older than lastKeyFrameUpdate.
          #if(DEBUG_POSE_HISTORY)
          AnkiInfo( "Localization.UpdatePoseWithKeyFrame.IgnoreOldKeyframe", "Ignoring keyframe %d at time %d\n", frameID, t);
          #endif
          return RESULT_OK;
        }

        // If origins match, then chain historical poses per frame up to the current pose
        if ( hist_[i].originID == originID )
        {
          // Start at the absolute localization pose we have recieved and add any pose information
          // added to history since its timestamp. This is done by chaining together the total pose
          // tranformations within each pose frame in history (time t to current).
          Embedded::Pose2d chainedPose(x, y, angle);

          Embedded::Pose2d poseAtStartOfFrame;
          Embedded::Pose2d poseAtEndOfFrame;
        
          u16 idx = i;
          PoseFrameID_t prevFrame = 0;
          // History is stored in a circular buffer so make sure idxAfterEnd will be valid
          const u16 idxAfterEnd = (hEnd_ == POSE_HISTORY_SIZE - 1 ? 0 : hEnd_ + 1);
          
          // Starting at time t go through history and find the starting and ending pose for each
          // frameID in history
          while(idx != idxAfterEnd)
          {
            poseAtStartOfFrame.x() = hist_[idx].x;
            poseAtStartOfFrame.y() = hist_[idx].y;
            poseAtStartOfFrame.angle() = hist_[idx].angle;
            
            // Increment idx until frameIDs in history change or we reach the end of history
            prevFrame = hist_[idx].frame;
            while(hist_[idx].frame == prevFrame && idx != idxAfterEnd)
            {
              prevFrame = hist_[idx].frame;
              
              if(++idx == POSE_HISTORY_SIZE)
              {
                idx = 0;
              }
            }
            
            // If we have reached the end of history then use our current pose
            if(idx == idxAfterEnd)
            {
              poseAtEndOfFrame.x() = x_;
              poseAtEndOfFrame.y() = y_;
              poseAtEndOfFrame.angle() = orientation_.ToFloat();
            }
            // Otherwise use the pose at the end of this frame
            else
            {
              // idx is pointing to the entry that has the incremented frameID so we need to subtract one to
              // point to the last entry for this frame. We also have to account for the fact that history
              // is stored in a circular buffer
              const u16 idxAtEndOfFrame = (idx == 0 ? POSE_HISTORY_SIZE - 1 : idx - 1);
              poseAtEndOfFrame.x() = hist_[idxAtEndOfFrame].x;
              poseAtEndOfFrame.y() = hist_[idxAtEndOfFrame].y;
              poseAtEndOfFrame.angle() = hist_[idxAtEndOfFrame].angle;
            }
            
            chainedPose = poseAtStartOfFrame.GetWithRespectTo(poseAtEndOfFrame) * chainedPose;
          }
          
          SetCurrPose(chainedPose.GetX(), chainedPose.GetY(), chainedPose.GetAngle());
        }
        else
        {
          SetCurrPose(x, y, angle);
          AddPoseToHist(); // Make sure this origin ID gets put in history
        }

        // We need to rely on t because we can receive several messages for the same t with minor HAL timer
        // differences. We want to process those messages as if they were of the same time (t)
        lastKeyframeUpdate_ = t;

        return RESULT_OK;
      }

      
      Result GetHistPoseAtIndex(u16 idx, Anki::Embedded::Pose2d& p) {
        if (idx >= POSE_HISTORY_SIZE) {
          return RESULT_FAIL;
        }

        p.x() = hist_[idx].x;
        p.y() = hist_[idx].y;
        p.angle() = hist_[idx].angle;

        return RESULT_OK;
      }

      Result GetHistPoseAtTime(TimeStamp_t t, Anki::Embedded::Pose2d& p)
      {
        // Check that there are actually poses in history
        if (hSize_ <= 0) {
          AnkiWarn( "Localization.GetHistPoseAtTime.NoHistory", "");
          return RESULT_FAIL;
        }

        // If the very first historical pose is newer than time t
        // then the time requested is too old.
        // Return the oldest historical pose just because it's better than nothing
        if (hist_[hStart_].t > t) {
          AnkiWarn( "Localization.GetHistPoseAtTime.TimeTooOld", "History starts at time %d, pose requested at time %d. Returning oldest pose.\n",
                   hist_[hStart_].t, t);
          GetHistPoseAtIndex(hStart_, p);
          return RESULT_FAIL;
        }

        // If the last historical pose is older than time t
        // the time requested is too new.
        // Return the newest histrical pose just because it's better than nothing
        if (hist_[hEnd_].t < t) {
          AnkiWarn( "Localization.GetHistPoseAtTime.TimeTooNew", "History ends at time %d, pose requested at time %d. Returning newest pose.\n",
                   hist_[hEnd_].t, t);
          GetHistPoseAtIndex(hEnd_, p);
          return RESULT_FAIL;
        }


        // Search through history for closest pose in time
        TimeStamp_t prevHistTime = 0;
        TimeStamp_t histTime = 0;
        u16 prevIdx = hStart_;

        for (u16 i = hStart_; prevIdx != hEnd_; ) {

          histTime = hist_[i].t;

          // See if there's an exact time match.
          // If so, return it.
          if (histTime == t) {
            GetHistPoseAtIndex(i, p);
            return RESULT_OK;
          }

          // Find the first historical pose that is newer than t
          if (histTime > t) {

            // Found first pose at time after t
            // Check if previous pose is closer to t than this one.
            // Return the closest one.
            if ((histTime - t) > (t - prevHistTime)) {
              GetHistPoseAtIndex(prevIdx, p);
            } else {
              GetHistPoseAtIndex(i, p);
            }
            return RESULT_OK;
          }

          prevHistTime = histTime;
          prevIdx = i;

          // Set i to next index
          if (i == POSE_HISTORY_SIZE-1) {
            i = 0;
          } else {
            ++i;
          }
        }

        return RESULT_FAIL;

      }


      /// ========= Localization ==========

      Result Init() {
        SetCurrPose(0,0,0);

        prevLeftWheelPos_ = HAL::MotorGetPosition(MotorID::MOTOR_LEFT_WHEEL);
        prevRightWheelPos_ = HAL::MotorGetPosition(MotorID::MOTOR_RIGHT_WHEEL);

        gyroRotOffset_ =  -IMUFilter::GetRotation();

        ClearHistory();

        return RESULT_OK;
      }

      
      f32 GetDriveCenterOffset()
      {
        // Get offset of the drive center from robot origin depending on carry state
        if (PickAndPlaceController::IsCarryingBlock()) {
          // If carrying a block the drive center goes forward, possibly to robot origin
          return 0;
        }

        return DRIVE_CENTER_OFFSET;
      }

      void Update()
      {

#if(USE_SIM_GROUND_TRUTH_POSE)
        // For initial testing only
        float angle;
        HAL::GetGroundTruthPose(x_,y_,angle);

        // Convert to mm
        x_ *= 1000;
        y_ *= 1000;

        orientation_ = angle;
#else

        // Update current pose estimate based on wheel motion

        f32 currLeftWheelPos = HAL::MotorGetPosition(MotorID::MOTOR_LEFT_WHEEL);
        f32 currRightWheelPos = HAL::MotorGetPosition(MotorID::MOTOR_RIGHT_WHEEL);

        // Compute distance traveled by each wheel
        f32 lDist = currLeftWheelPos - prevLeftWheelPos_;
        f32 rDist = currRightWheelPos - prevRightWheelPos_;

        // Compute new pose based on encoders and gyros, but only if there was any motion.
        bool movement = (!FLT_NEAR(rDist, 0.f) || !FLT_NEAR(lDist,0.f));
        
        // Consider there to be no movement if we're on the charger (VIC-1046).
        // This is to prevent the estimated pose from wandering if we're trying
        // to drive on the charger but slipping.
        if (HAL::BatteryIsOnCharger()) {
          movement = false;
        }
        
        if (movement) {
#if(DEBUG_LOCALIZATION)
          PRINT("\ncurrWheelPos (%f, %f)   prevWheelPos (%f, %f)\n",
                currLeftWheelPos, currRightWheelPos, prevLeftWheelPos_, prevRightWheelPos_);
#endif

          f32 lRadius, rRadius; // Radii of each wheel arc path (+ve radius means origin of arc is to the left)
          f32 cRadius; // Radius of arc traversed by center of robot
          f32 cDist;   // Distance traversed by center of robot
          f32 cTheta;  // Theta traversed by center of robot



          // lDist / lRadius = rDist / rRadius = theta
          // rRadius - lRadius = wheel_dist  => rRadius = wheel_dist + lRadius

          // lDist / lRadius = rDist / (wheel_dist + lRadius)
          // (wheel_dist + lRadius) / lRadius = rDist / lDist
          // wheel_dist / lRadius = rDist / lDist - 1
          // lRadius = wheel_dist / (rDist / lDist - 1)

          f32 wheelDistRatio = lDist / rDist;
          if ((rDist != 0) && (wheelDistRatio < 1.01f) && (wheelDistRatio > 0.99f)) {
//          if (FLT_NEAR(lDist, rDist)) {
            lRadius = BIG_RADIUS;
            rRadius = BIG_RADIUS;
            cRadius = BIG_RADIUS;
            cTheta = 0;
            cDist = lDist;
          } else {
            if (FLT_NEAR(lDist,0.f)) {
              lRadius = 0;
            } else {
              lRadius = WHEEL_DIST_MM / (rDist / lDist - 1);
            }
            rRadius = WHEEL_DIST_MM + lRadius;
            if (ABS(rRadius) > ABS(lRadius)) {
              cTheta = rDist / rRadius;
            } else {
              cTheta = lDist / lRadius;
            }
            cDist = 0.5f*(lDist + rDist);
            cRadius = lRadius + WHEEL_DIST_HALF_MM;
          }

#if(DEBUG_LOCALIZATION)
          PRINT("lRadius %f, rRadius %f, lDist %f, rDist %f, cTheta %f, cDist %f, cRadius %f\n",
                lRadius, rRadius, lDist, rDist, cTheta, cDist, cRadius);

          PRINT("oldPose: %f %f %f\n", x_, y_, orientation_.ToFloat());
#endif

          f32 driveCenterOffset = GetDriveCenterOffset();

          if (ABS(cRadius) >= BIG_RADIUS) {

            x_ += cDist * cosf(orientation_.ToFloat());
            y_ += cDist * sinf(orientation_.ToFloat());

            driveCenter_x_ = x_ + driveCenterOffset * cosf(orientation_.ToFloat());
            driveCenter_y_ = y_ + driveCenterOffset * sinf(orientation_.ToFloat());

            /*
            f32 dx = cDist * cosf(orientation_.ToFloat());
            f32 dy = cDist * sinf(orientation_.ToFloat());

            // Only update z position when moving straight
            if (onRamp_) {
              f32 pitch = IMUFilter::GetPitch();
              f32 cosp = cosf(pitch);
              x_ += dx * cosp;
              y_ += dy * cosp;
              z_ += cDist * tanf(pitch);
              PRINT("dx %f, dy %f, pitch %f  (z %f)\n", dx, dy, pitch, z_);
            } else {
              x_ += dx;
              y_ += dy;
            }
            */
          } else {

#if         (SLIP_MODELLING == 1)
            // Slip modelling method 1:
            // Assuming there is more slip of the slower tread than
            // there is on the faster one, thus making the total distance travelled
            // per tic closer to the distance traversed by the faster wheel than
            // their average distance.
            //
            // The lower the slipFactor the more the expected distance approaches
            // the distance expected of a two-wheeled no-slip robot.
            // The higher the value the more the expected distance approaches
            // the distance of the wheel that moved the most.
            // (i.e. Assumes the faster wheel drags the slower wheel along.
            // How correct is this assumption? Who knows?)
            //
            // When carrying a block, however, don't bother doing this since the
            // weight makes the robot drive more like a wheeled robot.
            // TODO: This is definitely not totally correct, but seems to be more
            //       right than not doing it, at least from what I can see from
            //       controlling it via the webots_keyboard_controller.
            if (!PickAndPlaceController::IsCarryingBlock()) {
              if (rDist * lDist >= 0) {
                // rDist and lDist are the same sign or at least one of them is zero
                f32 maxVal = MAX(ABS(lDist), ABS(rDist));
                if (cDist < 0) {
                  maxVal *= -1;
                }
                cDist = cDist * (1.f-slipFactor_) + (maxVal * slipFactor_);
              }
            }
            
#elif       (SLIP_MODELLING == 2)
            // Slip modelling method 2:
            // A simple discount factor applied to cDist.
            // The slipFactor represents the multiplier that is applied
            // at point turn. The multiplier linearly increases to 1 as
            // the ratio of the distances travelled by the two wheels approaches 1.
            f32 absWheelDistRatio = fabsf(wheelDistRatio);
            if (absWheelDistRatio > 1.f) {
              absWheelDistRatio = 1.f / absWheelDistRatio;
            }
            cDist *= slipFactor_ + (1.f-slipFactor_) * absWheelDistRatio;
            
#endif // SLIP_MODELLING
            

            // Get ICR offset from robot origin depending on carry state
            f32 driveCenterOffset = GetDriveCenterOffset();

            // Measure cTheta according to gyro
            // Clip according to expected cTheta since there's no way it can be more than that
            Radians newOrientation = IMUFilter::GetRotation() + gyroRotOffset_;
            f32 cTheta_meas = (newOrientation - orientation_).ToFloat();
            if (cTheta > 0) {
              cTheta_meas = CLIP(cTheta_meas, 0, cTheta);
            } else {
              cTheta_meas = CLIP(cTheta_meas, cTheta, 0);
            }
            

            // ======= Some experimental stuff to try later ========
           /*
            // Assuming that the actual distance travelled is the expected distance
            // travelled (based on encoders) scaled by the
            // measured rotation (cTheta_meas) / the expected rotation based on encoders (cTheta)
            // We only bother doing this if rotation is not too small.
            if (ABS(cTheta) > 0.003f) {
              cDist *= cTheta_meas / cTheta;
            }
             */

            
            // Compute distance traveled relative to previous position.
            // Drawing a straight line from the previous position to the new position forms a chord
            // in the circle defined by the turning radius as determined by the incremental wheel motion this tick.
            // The angle of this circle that this chord spans is cTheta.
            // The angle of the chord relative to the robot's previous trajectory is cTheta / 2.
            f32 cThetaHalf = 0.f;
            /*
            cThetaHalf = cTheta_meas * 0.5f;
            
            // The chord length is 2 * cRadius * sin(cThetaHalf).
            cDist = copysign((2 * cRadius * sinf(cThetaHalf)), cDist);
            */
            // ===============================================
            
            
            driveCenter_x_ = x_ + driveCenterOffset * cosf(orientation_.ToFloat());
            driveCenter_y_ = y_ + driveCenterOffset * sinf(orientation_.ToFloat());
            
            driveCenter_x_ += cDist * cosf(orientation_.ToFloat() + cThetaHalf);
            driveCenter_y_ += cDist * sinf(orientation_.ToFloat() + cThetaHalf);
            
            orientation_ = newOrientation;

            x_ = driveCenter_x_ - driveCenterOffset * cosf(orientation_.ToFloat());
            y_ = driveCenter_y_ - driveCenterOffset * sinf(orientation_.ToFloat());
            
          }

#if(DEBUG_LOCALIZATION)
          PRINT("newPose: %f %f %f\n", x_, y_, orientation_.ToFloat());
#endif

        }

        
        // Set orientation according to gyro
        orientation_ = IMUFilter::GetRotation() + gyroRotOffset_;

        prevLeftWheelPos_ = HAL::MotorGetPosition(MotorID::MOTOR_LEFT_WHEEL);
        prevRightWheelPos_ = HAL::MotorGetPosition(MotorID::MOTOR_RIGHT_WHEEL);
        

#if(USE_OVERLAY_DISPLAY)
        if(movement && HAL::GetTimeStamp()%100 == 0)
        {
          using namespace Sim::OverlayDisplay;

          SetText(CURR_EST_POSE, "Est. Pose: (x,y)=(%.4f, %.4f) at deg=%.1f",
                  x_, y_,
                  orientation_.getDegrees());

          HAL::GetGroundTruthPose(xTrue_, yTrue_, angleTrue_);
          Radians angleRad(angleTrue_);


          SetText(CURR_TRUE_POSE, "True Pose: (x,y)=(%.4f, %.4f) at deg=%.1f",
                  xTrue_ * 1000, yTrue_ * 1000, angleRad.getDegrees());

          prev_xTrue_ = xTrue_;
          prev_yTrue_ = yTrue_;
          prev_angleTrue_ = angleTrue_;

          UpdateEstimatedPose(x_, y_, orientation_.ToFloat());
        }
#endif



#endif  //else (!USE_SIM_GROUND_TRUTH_POSE)


        // Add new current pose to history
        static u8 addPoseToHistTicCnt = 0;
        if (++addPoseToHistTicCnt >= POSE_HISTORY_RES_IN_CYCLES) {
          AddPoseToHist();
          addPoseToHistTicCnt = 0;
        }

#if(DEBUG_LOCALIZATION)
        PRINT("LOC: %f, %f, %f\n", x_, y_, orientation_.getDegrees());
#endif
      }

      void SetCurrPose(const f32 &x, const f32 &y, const Radians &angle)
      {
        x_ = x;
        y_ = y;
        orientation_ = angle;
        gyroRotOffset_ = angle.ToFloat() - IMUFilter::GetRotation();

        // Update drive center pose
        f32 driveCenterOffset = GetDriveCenterOffset();
        driveCenter_x_ = x_ + driveCenterOffset * cosf(orientation_.ToFloat());
        driveCenter_y_ = y_ + driveCenterOffset * sinf(orientation_.ToFloat());

      } // SetCurrPose()

      void SetDriveCenterPose(const f32 &x, const f32 &y, const Radians &angle)
      {
        driveCenter_x_ = x;
        driveCenter_y_ = y;
        orientation_ = angle;
        gyroRotOffset_ = angle.ToFloat() - IMUFilter::GetRotation();

        // Update robot origin pose
        f32 driveCenterOffset = GetDriveCenterOffset();
        x_ = driveCenter_x_ - driveCenterOffset * cosf(orientation_.ToFloat());
        y_ = driveCenter_y_ - driveCenterOffset * sinf(orientation_.ToFloat());

      } // SetCurrPose()

      void GetCurrPose(f32& x, f32& y, Radians& angle)
      {
        x = x_;
        y = y_;
        angle = orientation_;
      } // GetCurrPose()

      
      f32 GetCurrPose_x()
      {
        return x_;
      }
      
      f32 GetCurrPose_y()
      {
        return y_;
      }
      
      const Radians& GetCurrPose_angle()
      {
        return orientation_;
      }
      
      Embedded::Pose2d GetCurrPose()
      {
        Embedded::Pose2d p(x_, y_, orientation_);
        return p;
      }

      void GetDriveCenterPose(f32& x, f32& y, Radians& angle)
      {
        x = driveCenter_x_;
        y = driveCenter_y_;
        angle = orientation_;
      }

      void ConvertToDriveCenterPose(const Anki::Embedded::Pose2d &robotOriginPose, Anki::Embedded::Pose2d &driveCenterPose)
      {
        f32 angle = robotOriginPose.GetAngle().ToFloat();

        driveCenterPose.x() = robotOriginPose.GetX() + GetDriveCenterOffset() * cosf(angle);
        driveCenterPose.y() = robotOriginPose.GetY() + GetDriveCenterOffset() * sinf(angle);
        driveCenterPose.angle() = robotOriginPose.GetAngle();
      }

      void ConvertToOriginPose(const Anki::Embedded::Pose2d &driveCenterPose, Anki::Embedded::Pose2d &robotOriginPose)
      {
        f32 angle = driveCenterPose.GetAngle().ToFloat();

        robotOriginPose.x() = driveCenterPose.GetX() - GetDriveCenterOffset() * cosf(angle);
        robotOriginPose.y() = driveCenterPose.GetY() - GetDriveCenterOffset() * sinf(angle);
        robotOriginPose.angle() = driveCenterPose.GetAngle();
      }


      PoseFrameID_t GetPoseFrameId()
      {
        return frameId_;
      }
      
      PoseOriginID_t GetPoseOriginId()
      {
        return originId_;
      }

      void ResetPoseFrame()
      {
        frameId_ = 0;
        ClearHistory();
      }

      f32 GetDistTo(const f32 x, const f32 y)
      {
        return sqrtf((x_-x)*(x_-x) + (y_-y)*(y_-y));
      
      }
      void SetMotionModelParams(f32 slipFactor)
      {
        slipFactor_ = slipFactor;
      }

    }
  }
}
