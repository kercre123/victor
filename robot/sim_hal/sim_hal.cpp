// System Includes
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <set>

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/faceDisplayDecode.h"
#include "util/logging/logging.h"
#include "messages.h"
#include "wheelController.h"

#include "anki/cozmo/simulator/robot/sim_overlayDisplay.h"
#include "clad/robotInterface/lightCubeMessage.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"


// Webots Includes
#include <webots/Robot.hpp>
#include <webots/Supervisor.hpp>
#include <webots/PositionSensor.hpp>
#include <webots/Emitter.hpp>
#include <webots/Motor.hpp>
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>
#include <webots/Camera.hpp>
#include <webots/Display.hpp>
#include <webots/Gyro.hpp>
#include <webots/DistanceSensor.hpp>
#include <webots/Accelerometer.hpp>
#include <webots/Receiver.hpp>
#include <webots/Connector.hpp>
#include <webots/LED.hpp>

#define BLUR_CAPTURED_IMAGES 1

#define DEBUG_GRIPPER 0

// Enable this to light up the backpack whenever a sound is being played
// When this is on, HAL::SetLED() doesn't work.
#define LIGHT_BACKPACK_DURING_SOUND 0


#if BLUR_CAPTURED_IMAGES
#include "opencv2/imgproc/imgproc.hpp"
#endif

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using sim_hal.cpp
#endif

namespace Anki {
  namespace Cozmo {

    namespace { // "Private members"

      // Const paramters / settings
      // TODO: some of these should be defined elsewhere (e.g. comms)

      const f64 WEBOTS_INFINITY = std::numeric_limits<f64>::infinity();

#pragma mark --- Simulated HardwareInterface "Member Variables" ---

      bool isInitialized = false;

      webots::Supervisor webotRobot_;

      s32 robotID_ = -1;

      // Motors
      webots::Motor* leftWheelMotor_;
      webots::Motor* rightWheelMotor_;
      bool usingTreads_ = false;

      webots::Motor* headMotor_;
      webots::Motor* liftMotor_;

      webots::Motor* motors_[MOTOR_COUNT];

      // Motor position sensors
      webots::PositionSensor* leftWheelPosSensor_;
      webots::PositionSensor* rightWheelPosSensor_;
      webots::PositionSensor* headPosSensor_;
      webots::PositionSensor* liftPosSensor_;
      webots::PositionSensor* motorPosSensors_[MOTOR_COUNT];

      // Gripper
      webots::Connector* con_;
      //bool gripperEngaged_ = false;
      bool isGripperEnabled_ = false;
      //s32 unlockhysteresis_ = UNLOCK_HYSTERESIS;


      // Cameras / Vision Processing
      bool enableVideo_;
      webots::Camera* headCam_;
      HAL::CameraInfo headCamInfo_;
      u32 cameraStartTime_ms_;

      // For pose information
      webots::GPS* gps_;
      webots::Compass* compass_;
      //webots::Node* estPose_;
      //char locStr[MAX_TEXT_DISPLAY_LENGTH];

      // IMU
      webots::Gyro* gyro_;
      webots::Accelerometer* accel_;

      // Prox sensors
      webots::DistanceSensor *proxCenter_;
      webots::DistanceSensor *cliffSensor_;
      
      // NOTE: Need more testing to figure out what these should be
      const u16 DROP_LEVEL = 400;
      const u16 UNDROP_LEVEL = 600;  // hysteresis
      bool cliffDetected_ = false;

      // Charge contact
      webots::Connector* chargeContact_;
      bool wasOnCharger_ = false;

      // Emitter / receiver for block communication
      webots::Receiver *objectDiscoveryReceiver_;
      webots::Emitter *blockCommsEmitter_;

      struct ActiveObjectSlotInfo {
        u32 assignedFactoryID;
        ActiveObjectType device_type;
        webots::Receiver *receiver;
        TimeStamp_t lastHeardTime;
        bool connected;
      };
      
      ActiveObjectSlotInfo activeObjectSlots_[MAX_NUM_ACTIVE_OBJECTS];
      
      // Flag to automatically connect to any block it discovers as long as it's
      // not already connected to a block of the same type.
      // This makes it nearly as if all the blocks in the world are already paired
      // with the robot in its flash.
      bool autoConnectToBlocks_;
      
      // For tracking wheel distance travelled
      f32 motorPositions_[MOTOR_COUNT];
      f32 motorPrevPositions_[MOTOR_COUNT];
      f32 motorSpeeds_[MOTOR_COUNT];
      f32 motorSpeedCoeffs_[MOTOR_COUNT];

      HAL::IDCard idCard_;

      // Lights
      webots::LED* leds_[NUM_BACKPACK_LEDS] = {0};

      #if(LIGHT_BACKPACK_DURING_SOUND)
      bool playingSound_ = false;
      #endif

      
      // Face display
      webots::Display* face_;
      const u32 DISPLAY_WIDTH = 128;
      const u32 DISPLAY_HEIGHT = 64;
      uint64_t faceFrame_[DISPLAY_WIDTH];

      s32 facePosX_ = 0;
      s32 facePosY_ = 0;

      // Audio
      // (Can't actually play sound in simulator, but proper handling of audio frames is still
      // necessary for proper animation timing)
      TimeStamp_t audioEndTime_ = 0;    // Expected end of audio
      u32 AUDIO_FRAME_TIME_MS = 33;     // Duration of single audio frame
      bool audioReadyForFrame_ = true;  // Whether or not ready to receive another audio frame
      
      u32 imageFrameID_ = 1;

#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      //void GetGlobalPose(f32 &x, f32 &y, f32& rad);


      // Approximate open-loop conversion of wheel power to angular wheel speed
      float WheelPowerToAngSpeed(float power)
      {
        // Inverse of speed-power formula in WheelController
        float speed_mm_per_s = power / 0.004f;

        if (usingTreads_) {
          // Return linear speed m/s when usingTreads
          return -speed_mm_per_s / 1000.f;
        }

        // Convert mm/s to rad/s
        return speed_mm_per_s / WHEEL_RAD_TO_MM;
      }

      // Approximate open-loop conversion of lift power to angular lift speed
      float LiftPowerToAngSpeed(float power)
      {
        // Inverse of speed-power formula in LiftController
        float rad_per_s = power / 0.05f;
        return rad_per_s;
      }
      
      // Approximate open-loop conversion of head power to angular head speed
      float HeadPowerToAngSpeed(float power)
      {
        return power * 2*PI_F;
      }


      void MotorUpdate()
      {
        // Update position and speed info
        f32 posDelta = 0;
        for(int i = 0; i < MOTOR_COUNT; i++)
        {
          if (motors_[i]) {
            f32 pos = motorPosSensors_[i]->getValue();
            if (usingTreads_) {
              if (i == MOTOR_LEFT_WHEEL || i == MOTOR_RIGHT_WHEEL) {
                pos = motorPosSensors_[i]->getValue() * -1000.f;
              }
            }

            posDelta = pos - motorPrevPositions_[i];

            // Update position
            motorPositions_[i] += posDelta;

            // Update speed
            motorSpeeds_[i] = (posDelta * ONE_OVER_CONTROL_DT) * (1.0 - motorSpeedCoeffs_[i]) + motorSpeeds_[i] * motorSpeedCoeffs_[i];

            motorPrevPositions_[i] = pos;
          }
        }
      }
      
      void AudioUpdate()
      {
        if (audioEndTime_ != 0) {
          if (HAL::GetTimeStamp() > audioEndTime_) {
            audioEndTime_ = 0;
            audioReadyForFrame_ = true;
          } else if (HAL::GetTimeStamp() > audioEndTime_ - (0.5*AUDIO_FRAME_TIME_MS)) {
            // Audio ready flag is raised ~16ms before the end of the current frame.
            // This means audio lags other tracks but the amount should be imperceptible.
            audioReadyForFrame_ = true;
          }
          
          #if(LIGHT_BACKPACK_DURING_SOUND)
          if (playingSound_ && audioEndTime_ != 0) {
            leds_[LED_BACKPACK_BACK]->set(0x00ff0000);
          } else {
            leds_[LED_BACKPACK_BACK]->set(0x0);
          }
          #endif
        }
      }

      Result SendBlockMessage(const u8 activeID, const BlockMessages::LightCubeMessage& msg)
      {
        if (activeID >= MAX_NUM_ACTIVE_OBJECTS) {
          return RESULT_FAIL;
        }
        
        if (activeObjectSlots_[activeID].connected) {
          blockCommsEmitter_->setChannel( activeObjectSlots_[activeID].assignedFactoryID + 1 );
          blockCommsEmitter_->send(msg.GetBuffer(), msg.Size());
        }
        return RESULT_OK;
      }


    } // "private" namespace

    namespace Sim {
      // Create a pointer to the webots supervisor object within
      // a Simulator namespace so that other Simulation-specific code
      // can talk to it.  This avoids there being a global gCozmoBot
      // running around, accessible in non-simulator code.
      webots::Supervisor* CozmoBot = &webotRobot_;
    }

#pragma mark --- Simulated Hardware Method Implementations ---

    // Forward Declaration.  This is implemented in sim_radio.cpp
    Result InitSimRadio(const char* advertisementIP);

    Result HAL::Init()
    {
      assert(TIME_STEP >= webotRobot_.getBasicTimeStep());

      webots::Node* robotNode = webotRobot_.getSelf();
      usingTreads_ = robotNode->getField("useTreads")->getSFBool();

      leftWheelMotor_  = webotRobot_.getMotor("LeftWheelMotor");
      rightWheelMotor_ = webotRobot_.getMotor("RightWheelMotor");

      headMotor_  = webotRobot_.getMotor("HeadMotor");
      liftMotor_  = webotRobot_.getMotor("LiftMotor");

      leftWheelPosSensor_ = webotRobot_.getPositionSensor("LeftWheelMotorPosSensor");
      rightWheelPosSensor_ = webotRobot_.getPositionSensor("RightWheelMotorPosSensor");

      headPosSensor_ = webotRobot_.getPositionSensor("HeadMotorPosSensor");
      liftPosSensor_ = webotRobot_.getPositionSensor("LiftMotorPosSensor");


      con_ = webotRobot_.getConnector("gripperConnector");
      //con_->enablePresence(TIME_STEP);

      headCam_ = webotRobot_.getCamera("HeadCamera");

      if(VISION_TIME_STEP % static_cast<u32>(webotRobot_.getBasicTimeStep()) != 0) {
        PRINT("VISION_TIME_STEP (%d) must be a multiple of the world's basic timestep (%.0f).\n",
              VISION_TIME_STEP, webotRobot_.getBasicTimeStep());
        return RESULT_FAIL;
      }
      headCam_->enable(VISION_TIME_STEP);

      // HACK: Figure out when first camera image will actually be taken (next
      // timestep from now), so we can reference to it when computing frame
      // capture time from now on.
      // TODO: Not sure from Cyberbotics support message whether this should include "+ TIME_STEP" or not...
      cameraStartTime_ms_ = HAL::GetTimeStamp(); // + TIME_STEP;
      PRINT_NAMED_INFO("SIM", "Setting camera start time as %d", cameraStartTime_ms_);

      enableVideo_ = robotNode->getField("enableVideo")->getSFBool();
      if (!enableVideo_) {
        printf("WARN: ******** Video disabled *********\n");
      }

      // Set ID
      // Expected format of name is <SomeName>_<robotID>
      std::string name = webotRobot_.getName();
      size_t lastDelimPos = name.rfind('_');
      if (lastDelimPos != std::string::npos) {
        robotID_ = atoi( name.substr(lastDelimPos+1).c_str() );
        if (robotID_ < 1) {
          PRINT_NAMED_ERROR("SIM.RobotID", "Invalid robot name (%s). ID must be greater than 0.", name.c_str());
          return RESULT_FAIL;
        }
        PRINT_NAMED_INFO("SIM", "Initializing robot ID: %d", robotID_);
      } else {
        PRINT_NAMED_ERROR("SIM.RobotName", "Cozmo robot name %s is invalid.  Must end with '_<ID number>'.", name.c_str());
        return RESULT_FAIL;
      }

      // ID card info
      idCard_.esn = robotID_;
      idCard_.modelNumber = 0;
      idCard_.lotCode = 0;
      idCard_.birthday = 0;
      idCard_.hwVersion = 0;


      //Set the motors to velocity mode
      headMotor_->setPosition(WEBOTS_INFINITY);
      liftMotor_->setPosition(WEBOTS_INFINITY);
      leftWheelMotor_->setPosition(WEBOTS_INFINITY);
      rightWheelMotor_->setPosition(WEBOTS_INFINITY);

      // Load motor array
      motors_[MOTOR_LEFT_WHEEL] = leftWheelMotor_;
      motors_[MOTOR_RIGHT_WHEEL] = rightWheelMotor_;
      motors_[MOTOR_HEAD] = headMotor_;
      motors_[MOTOR_LIFT] = liftMotor_;
      //motors_[MOTOR_GRIP] = NULL;

      // Load position sensor array
      motorPosSensors_[MOTOR_LEFT_WHEEL] = leftWheelPosSensor_;
      motorPosSensors_[MOTOR_RIGHT_WHEEL] = rightWheelPosSensor_;
      motorPosSensors_[MOTOR_HEAD] = headPosSensor_;
      motorPosSensors_[MOTOR_LIFT] = liftPosSensor_;


      // Initialize motor positions
      for (int i=0; i < MOTOR_COUNT; ++i) {
        motorPositions_[i] = 0;
        motorPrevPositions_[i] = 0;
        motorSpeeds_[i] = 0;
        motorSpeedCoeffs_[i] = 0.5;
      }

      // Enable position measurements on head, lift, and wheel motors
      leftWheelPosSensor_->enable(TIME_STEP);
      rightWheelPosSensor_->enable(TIME_STEP);

      headPosSensor_->enable(TIME_STEP);
      liftPosSensor_->enable(TIME_STEP);

      // Set speeds to 0
      leftWheelMotor_->setVelocity(0);
      rightWheelMotor_->setVelocity(0);
      headMotor_->setVelocity(0);
      liftMotor_->setVelocity(0);

      // Get localization sensors
      gps_ = webotRobot_.getGPS("gps");
      compass_ = webotRobot_.getCompass("compass");
      gps_->enable(TIME_STEP);
      compass_->enable(TIME_STEP);
      //estPose_ = webotRobot_.getFromDef("CozmoBotPose");

      // Gyro
      gyro_ = webotRobot_.getGyro("gyro");
      gyro_->enable(TIME_STEP);

      // Accelerometer
      accel_ = webotRobot_.getAccelerometer("accel");
      accel_->enable(TIME_STEP);

      // Proximity sensors
      proxCenter_ = webotRobot_.getDistanceSensor("forwardProxSensor");
      cliffSensor_ = webotRobot_.getDistanceSensor("cliffSensor");
      
      proxCenter_->enable(TIME_STEP);
      cliffSensor_->enable(TIME_STEP);

      // Charge contact
      chargeContact_ = webotRobot_.getConnector("ChargeContact");
      chargeContact_->enablePresence(TIME_STEP);
      wasOnCharger_ = false;


      // Block radio
      objectDiscoveryReceiver_ = webotRobot_.getReceiver("discoveryReceiver");
      objectDiscoveryReceiver_->setChannel(OBJECT_DISCOVERY_CHANNEL);
      objectDiscoveryReceiver_->enable(TIME_STEP);
      
      blockCommsEmitter_ = webotRobot_.getEmitter("blockCommsEmitter");
      
      char receiverName[32];
      for (int i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
        sprintf(receiverName, "blockCommsReceiver%d", i);
        activeObjectSlots_[i].receiver = webotRobot_.getReceiver(receiverName);
        activeObjectSlots_[i].assignedFactoryID = 0;
        activeObjectSlots_[i].lastHeardTime = 0;
        activeObjectSlots_[i].connected = false;
      }

      // Whether or not to automatically connect to advertising blocks
      autoConnectToBlocks_ = robotNode->getField("autoConnectToBlocks")->getSFBool();
      

      // Get advertisement host IP
      webots::Field *advertisementHostField = webotRobot_.getSelf()->getField("advertisementHost");
      std::string advertisementIP = "127.0.0.1";
      if (advertisementHostField) {
        advertisementIP = advertisementHostField->getSFString();
      } else {
        printf("No valid advertisement IP found\n");
      }

      if(InitSimRadio(advertisementIP.c_str()) == RESULT_FAIL) {
        printf("Failed to initialize Simulated Radio.\n");
        return RESULT_FAIL;
      }

      // Lights
      /* Old eye LED segments
      leds_[LED_LEFT_EYE_TOP] = webotRobot_.getLED("LeftEyeLED_top");
      leds_[LED_LEFT_EYE_LEFT] = webotRobot_.getLED("LeftEyeLED_left");
      leds_[LED_LEFT_EYE_RIGHT] = webotRobot_.getLED("LeftEyeLED_right");
      leds_[LED_LEFT_EYE_BOTTOM] = webotRobot_.getLED("LeftEyeLED_bottom");

      leds_[LED_RIGHT_EYE_TOP] = webotRobot_.getLED("RightEyeLED_top");
      leds_[LED_RIGHT_EYE_LEFT] = webotRobot_.getLED("RightEyeLED_left");
      leds_[LED_RIGHT_EYE_RIGHT] = webotRobot_.getLED("RightEyeLED_right");
      leds_[LED_RIGHT_EYE_BOTTOM] = webotRobot_.getLED("RightEyeLED_bottom");
      */

      leds_[LED_BACKPACK_BACK]   = webotRobot_.getLED("ledHealth0");
      leds_[LED_BACKPACK_MIDDLE] = webotRobot_.getLED("ledHealth1");
      leds_[LED_BACKPACK_FRONT]  = webotRobot_.getLED("ledHealth2");
      leds_[LED_BACKPACK_LEFT]   = webotRobot_.getLED("ledDirLeft");
      leds_[LED_BACKPACK_RIGHT]  = webotRobot_.getLED("ledDirRight");

      // Face display
      face_ = webotRobot_.getDisplay("face_display");
      assert(face_->getWidth() == DISPLAY_WIDTH);
      assert(face_->getHeight() == DISPLAY_HEIGHT);
      FaceClear();

      isInitialized = true;
      return RESULT_OK;

    } // Init()

    void HAL::Destroy()
    {
      // Turn off components: (strictly necessary?)
      headCam_->disable();

      gps_->disable();
      compass_->disable();

    } // Destroy()

    bool HAL::IsInitialized(void)
    {
      return isInitialized;
    }


    void HAL::GetGroundTruthPose(f32 &x, f32 &y, f32& rad)
    {
      const double* position = gps_->getValues();
      const double* northVector = compass_->getValues();

      x = position[0];
      y = position[1];

      rad = std::atan2(-northVector[1], northVector[0]);

      //PRINT("GroundTruth:  pos %f %f %f   rad %f %f %f\n", position[0], position[1], position[2],
      //      northVector[0], northVector[1], northVector[2]);


    } // GetGroundTruthPose()


    bool HAL::IsGripperEngaged() {
      return isGripperEnabled_ && con_->getPresence()==1;
    }

    void HAL::UpdateDisplay(void)
    {
      using namespace Sim::OverlayDisplay;
     /*
      PRINT("speedDes: %d, speedCur: %d, speedCtrl: %d, speedMeas: %d\n",
              GetUserCommandedDesiredVehicleSpeed(),
              GetUserCommandedCurrentVehicleSpeed(),
              GetControllerCommandedVehicleSpeed(),
              GetCurrentMeasuredVehicleSpeed());
      */

    } // HAL::UpdateDisplay()

    
    bool HAL::IMUReadData(HAL::IMU_DataStructure &IMUData)
    {
      const double* vals = gyro_->getValues();  // rad/s
      IMUData.rate_x = (f32)(vals[0]);
      IMUData.rate_y = (f32)(vals[1]);
      IMUData.rate_z = (f32)(vals[2]);

      vals = accel_->getValues();   // m/s^2
      IMUData.acc_x = (f32)(vals[0] * 1000);  // convert to mm/s^2
      IMUData.acc_y = (f32)(vals[1] * 1000);
      IMUData.acc_z = (f32)(vals[2] * 1000);
      
      // Return true if IMU was already read this timestamp
      static TimeStamp_t lastReadTimestamp = 0;
      bool newReading = lastReadTimestamp != HAL::GetTimeStamp();
      lastReadTimestamp = HAL::GetTimeStamp();
      return newReading;
    }
    
    void HAL::IMUReadRawData(int16_t* accel, int16_t* gyro, uint8_t* timestamp)
    {
      // Just storing junk values since this function exists purely for HW debug
      *timestamp = HAL::GetTimeStamp() % u8_MAX;
      accel[0] = accel[1] = accel[2] = 0;
      gyro[0] = gyro[1] = gyro[2] = 0;
    }


    // Set the motor power in the unitless range [-1.0, 1.0]
    void HAL::MotorSetPower(MotorID motor, f32 power)
    {
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
          leftWheelMotor_->setVelocity(WheelPowerToAngSpeed(power));
          break;
        case MOTOR_RIGHT_WHEEL:
          rightWheelMotor_->setVelocity(WheelPowerToAngSpeed(power));
          break;
        case MOTOR_LIFT:
          liftMotor_->setVelocity(LiftPowerToAngSpeed(power));
          break;
        case MOTOR_HEAD:
          // TODO: Assuming linear relationship, but it's not!
          headMotor_->setVelocity(HeadPowerToAngSpeed(power));
          break;
        default:
          PRINT("ERROR (HAL::MotorSetPower) - Undefined motor type %d\n", motor);
          return;
      }
    }

    // Reset the internal position of the specified motor to 0
    void HAL::MotorResetPosition(MotorID motor)
    {
      if (motor >= MOTOR_COUNT) {
        PRINT("ERROR (HAL::MotorResetPosition) - Undefined motor type %d\n", motor);
        return;
      }

      motorPositions_[motor] = 0;
      //motorPrevPositions_[motor] = 0;
    }

    // Returns units based on the specified motor type:
    // Wheels are in mm/s, everything else is in degrees/s.
    f32 HAL::MotorGetSpeed(MotorID motor)
    {
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
        case MOTOR_RIGHT_WHEEL:
          if (!usingTreads_) {
            return motorSpeeds_[motor] * WHEEL_RAD_TO_MM;
          }
          // else if usingTreads, fall through to just returning motorSpeeds_ since
          // it is already stored in mm/s

        case MOTOR_LIFT:
        case MOTOR_HEAD:
          return motorSpeeds_[motor];

        default:
          PRINT("ERROR (HAL::MotorGetSpeed) - Undefined motor type %d\n", motor);
          break;
      }
      return 0;
    }

    // Returns units based on the specified motor type:
    // Wheels are in mm since reset, everything else is in degrees.
    f32 HAL::MotorGetPosition(MotorID motor)
    {
      switch(motor) {
        case MOTOR_RIGHT_WHEEL:
        case MOTOR_LEFT_WHEEL:
          if (!usingTreads_) {
            return motorPositions_[motor] * WHEEL_RAD_TO_MM;
          }
          // else if usingTreads, fall through to just returning motorSpeeds_ since
          // it is already stored in mm

        case MOTOR_LIFT:
        case MOTOR_HEAD:
          return motorPositions_[motor];

        default:
          PRINT("ERROR (HAL::MotorGetPosition) - Undefined motor type %d\n", motor);
          return 0;
      }

      return 0;
    }


    void HAL::EngageGripper()
    {
      con_->lock();
      con_->enablePresence(TIME_STEP);
      isGripperEnabled_ = true;
#     if DEBUG_GRIPPER
      PRINT("GRIPPER LOCKED!\n");
#     endif

      /*
      //Should we lock to a block which is close to the connector?
      if (!gripperEngaged_ && con_->getPresence() == 1)
      {
        if (unlockhysteresis_ == 0)
        {
          con_->lock();
          gripperEngaged_ = true;
          printf("GRIPPER LOCKED!\n");
        }else{
          unlockhysteresis_--;
        }
      }
       */
    }

    void HAL::DisengageGripper()
    {
      con_->unlock();
      con_->disablePresence();
      isGripperEnabled_ = false;
#     if DEBUG_GRIPPER
      PRINT("GRIPPER UNLOCKED!\n");
#     endif

      /*
      if (gripperEngaged_)
      {
        gripperEngaged_ = false;
        unlockhysteresis_ = UNLOCK_HYSTERESIS;
        con_->unlock();
        printf("GRIPPER UNLOCKED!\n");
      }
       */
    }

    bool IsSameTypeActiveObjectAssigned(u32 device_type)
    {
      ASSERT_NAMED_EVENT(device_type != 0, "sim_hal.IsSameTypeActiveObjectAssigned.InvalidType", "");
      
      for (u32 i = 0; i < MAX_NUM_ACTIVE_OBJECTS; ++i) {
        if (activeObjectSlots_[i].assignedFactoryID != 0 && activeObjectSlots_[i].device_type == device_type) {
          return true;
        }
      }
      return false;
    }


    // Forward declaration
    void RadioUpdate();
    void ActiveObjectsUpdate();
    void SendObjectConnectionState(u32 slot_id);

    Result HAL::Step(void)
    {

      if(webotRobot_.step(Cozmo::TIME_STEP) == -1) {
        return RESULT_FAIL;
      } else {
        MotorUpdate();
        RadioUpdate();
        AudioUpdate();
        ActiveObjectsUpdate();

        /*
        // Always display ground truth pose:
        {
          const double* position = gps_->getValues();
          const double* northVector = compass_->getValues();

          const f32 rad = std::atan2(-northVector[1], northVector[0]);

          char buffer[256];
          snprintf(buffer, 256, "Robot %d Pose: (%.1f,%.1f,%.1f), %.1fdeg@(0,0,1)",
                   robotID_,
                   M_TO_MM(position[0]), M_TO_MM(position[1]), M_TO_MM(position[2]),
                   RAD_TO_DEG(rad));

          std::string poseString(buffer);
          webotRobot_.setLabel(robotID_, poseString, 0.5, robotID_*.05, .05, 0xff0000, 0.);
        }
         */
        
        
        // Send block connection state when engine connects
        static bool wasConnected = false;
        if (!wasConnected && HAL::RadioIsConnected()) {

          // Send RobotAvailable indicating sim robot
          RobotInterface::RobotAvailable idMsg;
          idMsg.robotID = 0;
          idMsg.modelID = 0;
          RobotInterface::SendMessage(idMsg);

          
          // send firmware info indicating simulated robot
          {
            std::string firmwareJson{"{\"version\":0,\"time\":0,\"sim\":1}"};
            RobotInterface::FirmwareVersion msg;
            msg.RESRVED = 0;
            msg.json_length = firmwareJson.size() + 1;
            std::memcpy(msg.json, firmwareJson.c_str(), firmwareJson.size() + 1);
            RobotInterface::SendMessage(msg);
          }

          // Send info about connected active objects
          for (int i = 0; i < MAX_NUM_ACTIVE_OBJECTS; ++i) {
            if (activeObjectSlots_[i].connected) {
              SendObjectConnectionState(i);
            }
          }
          wasConnected = true;
        } else if (wasConnected && !HAL::RadioIsConnected()) {
          wasConnected = false;
        }

        // Check charging status (Debug)
        if (BatteryIsOnCharger() && !wasOnCharger_) {
          PRINT("ON CHARGER\n");
          wasOnCharger_ = true;
        } else if (!BatteryIsOnCharger() && wasOnCharger_) {
          PRINT("OFF CHARGER\n");
          wasOnCharger_ = false;
        }

        return RESULT_OK;
      }


    } // step()


    // Helper function to create a CameraInfo struct from Webots camera properties:
    void FillCameraInfo(const webots::Camera *camera,
                        HAL::CameraInfo &info)
    {

      const u16 nrows  = static_cast<u16>(camera->getHeight());
      const u16 ncols  = static_cast<u16>(camera->getWidth());
      const f32 width  = static_cast<f32>(ncols);
      const f32 height = static_cast<f32>(nrows);
      //f32 aspect = width/height;

      const f32 fov_hor = camera->getFov();

      // Compute focal length from simulated camera's reported FOV:
      const f32 f = width / (2.f * std::tan(0.5f*fov_hor));

      // There should only be ONE focal length, because simulated pixels are
      // square, so no need to compute/define a separate fy
      //f32 fy = height / (2.f * std::tan(0.5f*fov_ver));

      info.focalLength_x = f;
      info.focalLength_y = f;
      info.center_x      = 0.5f*(width-1);
      info.center_y      = 0.5f*(height-1);
      info.skew          = 0.f;
      info.nrows         = nrows;
      info.ncols         = ncols;

      for(u8 i=0; i<NUM_RADIAL_DISTORTION_COEFFS; ++i) {
        info.distortionCoeffs[i] = 0.f;
      }

    } // FillCameraInfo

    const HAL::CameraInfo* HAL::GetHeadCamInfo(void)
    {
      if(isInitialized) {
        FillCameraInfo(headCam_, headCamInfo_);
        return &headCamInfo_;
      }
      else {
        PRINT("HeadCam calibration requested before HAL initialized.\n");
        return NULL;
      }
    }

    /*
    HAL::CameraMode HAL::GetHeadCamMode(void)
    {
      return headCamMode_;
    }

    // TODO: there is a copy of this in hal.cpp -- consolidate into one location.
    // TODO: perhaps we'd rather have this be a switch statement
    //       (However, if the header is stored as a member of the CameraModeInfo
    //        struct, we can't use it as a case in the switch statement b/c
    //        the compiler doesn't think it's a constant expression.  We can
    //        get around this using "constexpr" when declaring CameraModeInfo,
    //        but that's a C++11 thing and not likely supported on the Movidius
    //        compiler)
    void HAL::SetHeadCamMode(const u8 frameResHeader)
    {
      bool found = false;
      for(CameraMode mode = CAMERA_MODE_VGA;
          not found && mode != CAMERA_MODE_COUNT; ++mode)
      {
        if(frameResHeader == CameraModeInfo[mode].header) {
          headCamMode_ = mode;
          found = true;
        }
      }

      if(not found) {
        PRINT("ERROR(SetCameraMode): Unknown frame res: %d", frameResHeader);
      }
    } //SetHeadCamMode()
    */
    
    void HAL::CameraGetDefaultParameters(DefaultCameraParams& params)
    {
      params.minExposure_ms = 0;
      params.maxExposure_ms = 67;
      params.gain = 2.f;
      params.maxGain = 4.f;
      
      u8 count = 0;
      for(u8 i = 0; i < GAMMA_CURVE_SIZE; ++i)
      {
        params.gammaCurve[i] = count;
        count += 255/GAMMA_CURVE_SIZE;
      }
      
      return;
    }

    void HAL::CameraSetParameters(u16 exposure_ms, f32 gain)
    {
      // Can't control simulated camera's exposure.

      // TODO: Simulate this somehow?

      return;

    } // HAL::CameraSetParameters()


    u32 HAL::GetCameraStartTime()
    {
      return cameraStartTime_ms_;
    }

    bool HAL::IsVideoEnabled()
    {
      return enableVideo_;
    }

    // Starts camera frame synchronization
    void HAL::CameraGetFrame(u8* frame, ImageResolution res, bool enableLight)
    {
      // TODO: enableLight?
      AnkiConditionalErrorAndReturn(frame != NULL, 190, "SimHAL.CameraGetFrame.NullFramePointer", 494, "NULL frame pointer provided to CameraGetFrame(), check "
                                    "to make sure the image allocation succeeded.\n", 0);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      static u32 lastFrameTime_ms = static_cast<u32>(webotRobot_.getTime()*1000.0);
      u32 currentTime_ms = static_cast<u32>(webotRobot_.getTime()*1000.0);
      AnkiConditionalWarn(currentTime_ms - lastFrameTime_ms > headCam_->getSamplingPeriod(), 191, "SimHAL.CameraGetFrame", 495, "Image requested too soon -- new frame may not be ready yet.\n", 0);
#endif

      const u8* image = headCam_->getImage();

      AnkiConditionalErrorAndReturn(image != NULL, 192, "SimHAL.CameraGetFrame.NullImagePointer", 496, "NULL image pointer returned from simulated camera's getFrame() method.\n", 0);

      s32 pixel = 0;
      s32 imgWidth = headCam_->getWidth();
      for (s32 y=0; y < headCamInfo_.nrows; y++) {
        for (s32 x=0; x < headCamInfo_.ncols; x++) {
          frame[pixel++] = webots::Camera::imageGetRed(image, imgWidth, x, y);
          frame[pixel++] = webots::Camera::imageGetGreen(image, imgWidth, x, y);
          frame[pixel++] = webots::Camera::imageGetBlue(image,  imgWidth, x, y);
        }
      }

#     if BLUR_CAPTURED_IMAGES
      // Add some blur to simulated images
      cv::Mat cvImg(headCamInfo_.nrows, headCamInfo_.ncols, CV_8UC3, frame);
      cv::GaussianBlur(cvImg, cvImg, cv::Size(0,0), 0.75f);
#     endif

      imageFrameID_++;

    } // CameraGetFrame()
    
    void HAL::IMUGetCameraTime(uint32_t* const frameNumber, uint8_t* const line2Number)
    {
      static uint32_t prevImageID_ = 0;
      static uint8_t line2Number_ = 0;
      
      // line2Number number increases while a frame is being captured so subsequent calls to this function where
      // imageFrameID_ hasn't changed should increase line2Number_
      // This function is not called more than 4 times per frame so line2Number_ will never exceed (60 + 1) * 4 = 244
      // where the max value it could be is 250 (500 scanlines per frame / 2)
      uint8_t line2NumberIncrement = HAL::GetTimeStamp() % 60 + 1;
      if(prevImageID_ == imageFrameID_)
      {
        line2Number_ += line2NumberIncrement;
      }
      else
      {
        line2Number_ = line2NumberIncrement;
      }
    
      *frameNumber = imageFrameID_;
      *line2Number = line2Number_;
      
      prevImageID_ = imageFrameID_;
    }
    
    // Get the number of microseconds since boot
    u32 HAL::GetMicroCounter(void)
    {
      return static_cast<u32>(webotRobot_.getTime() * 1000000.0);
    }

    void HAL::MicroWait(u32 microseconds)
    {
      u32 now = GetMicroCounter();
      while ((GetMicroCounter() - now) < microseconds)
        ;
    }

    TimeStamp_t HAL::GetTimeStamp(void)
    {
      return static_cast<TimeStamp_t>(webotRobot_.getTime() * 1000.0);
      //return timeStamp_;
    }

    void HAL::SetTimeStamp(TimeStamp_t t)
    {
      //timeStamp_ = t;
    };

    void HAL::SetLED(LEDId led_id, u16 color) {
      #if(!LIGHT_BACKPACK_DURING_SOUND)
      if (leds_[led_id]) {
        leds_[led_id]->set( ((color & LED_ENC_IR) ? LED_IR : 0) |
                           (((color & LED_ENC_RED) >> LED_ENC_RED_SHIFT) << (16 + 3)) |
                           (((color & LED_ENC_GRN) >> LED_ENC_GRN_SHIFT) << ( 8 + 3)) |
                           (((color & LED_ENC_BLU) >> LED_ENC_BLU_SHIFT) << ( 0 + 3)));
      } else {
        PRINT("Unhandled LED %d\n", led_id);
      }
      #endif
    }

    void HAL::SetHeadlights(bool state)
    {
      // TODO: ...
    }

    void HAL::AudioFill(void) {}

    // @return true if the audio clock says it is time for the next frame
    bool HAL::AudioReady()
    {
      return audioReadyForFrame_;
    }

    void HAL::AudioPlaySilence() {
      AudioPlayFrame(nullptr);
    }

    // Play one frame of audio or silence
    // @param frame - a pointer to an audio frame or NULL to play one frame of silence
    void HAL::AudioPlayFrame(AnimKeyFrame::AudioSample *msg)
    {
      if (audioEndTime_ == 0) {
        audioEndTime_ = HAL::GetTimeStamp();
      }
      audioEndTime_ += AUDIO_FRAME_TIME_MS;
      audioReadyForFrame_ = false;
      
      #if(LIGHT_BACKPACK_DURING_SOUND)
      playingSound_ = msg == nullptr ? false : true;
      #endif
    }

    void HAL::FaceClear()
    {
      face_->setColor(0);
      face_->fillRectangle(0,0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
      face_->setColor(0x0000f0ff);
    }

    void HAL::FaceAnimate(u8* src, const u16 length)
    {
      // Clear the display
      FaceClear();

      // Decode the image
      if (length == MAX_FACE_FRAME_SIZE) memcpy(faceFrame_, src, MAX_FACE_FRAME_SIZE);
      else FaceDisplayDecode(src, DISPLAY_HEIGHT, DISPLAY_WIDTH, faceFrame_);

      // Draw face
      FaceMove(facePosX_, facePosY_);
    }

    // Move the face to an X, Y offset - where 0, 0 is centered, negative is left/up
    void HAL::FaceMove(s32 x, s32 y)
    {
      // Compute starting pixel of dest image (i.e. face_ display)
      u8 destX = 0, destY = 0;

      if (x > 0) {
        destX += x;
      }

      if (y > 0) {
        destY += y;
      }

      // Compute dimensions of overlapping region
      u8 w = DISPLAY_WIDTH - ABS(x);
      u8 h = DISPLAY_HEIGHT - ABS(y);

      FaceClear();

      for (u8 i = 0; i < w; ++i) {
        for (u8 j = 0; j < h; ++j) {
          if ((faceFrame_[i] >> j) & 1) {
            face_->drawPixel(destX + i, destY + j);
          }
        }
      }

      facePosX_ = x;
      facePosY_ = y;

    }

    HAL::IDCard* HAL::GetIDCard()
    {
      return &idCard_;
    }
    
    u32 HAL::GetID()
    {
      return 0;
    }

    u8 HAL::GetForwardProxSensorCurrentValue()
    {
      const u8 val = static_cast<u8>( proxCenter_->getValue() );
      return val;
    }
    
    bool HAL::IsCliffDetected()
    {
      u16 cliffLevel = static_cast<u16>(cliffSensor_->getValue());
      
      if (!cliffDetected_ && cliffLevel < DROP_LEVEL) {
        cliffDetected_ = true;
      } else if (cliffDetected_ && cliffLevel > UNDROP_LEVEL) {
        cliffDetected_ = false;
      }
      return cliffDetected_;
    }

    u16 HAL::GetRawCliffData()
    {
      return static_cast<u16>(cliffSensor_->getValue());
    }
    
    namespace HAL {
      int UARTGetFreeSpace()
      {
        return 100000000;
      }

      int UARTGetWriteBufferSize()
      {
        return 100000000;
      }
    }

    f32 HAL::BatteryGetVoltage()
    {
      return 5;
    }

    bool HAL::BatteryIsCharging()
    {
      //return false; // XXX On Cozmo 3, head is off if robot is charging
      return (chargeContact_->getPresence() == 1);
    }

    bool HAL::BatteryIsOnCharger()
    {
      //return false; // XXX On Cozmo 3, head is off if robot is charging
      return (chargeContact_->getPresence() == 1);
    }
    
    bool HAL::BatteryIsChargerOOS()
    {
      return false;
    }

    extern "C" {
    void EnableIRQ() {}
    void DisableIRQ() {}
    }

    void HAL::FlashBlockIDs() {
      // TODO: Fix this if and when we know how it'll work in HW
    }

    Result HAL::SetBlockLight(const u32 activeID, const u16* colors)
    {
      BlockMessages::LightCubeMessage m;
      m.tag = BlockMessages::LightCubeMessage::Tag_setCubeLights;
      for (int i=0; i<NUM_CUBE_LEDS; ++i) {
        m.setCubeLights.lights[i].onColor = colors[i];
      }
      
      BlockMessages::LightCubeMessage msg;
      msg.tag = BlockMessages::LightCubeMessage::Tag_setCubeID;
      msg.setCubeID.objectID = activeID;
      msg.setCubeID.rotationPeriod_frames = 0;
      
      SendBlockMessage(activeID, msg);
      return SendBlockMessage(activeID, m);
    }

    
    void SendObjectConnectionState(u32 slot_id)
    {
      if (slot_id >= MAX_NUM_ACTIVE_OBJECTS) {
        return;
      }
      
      ActiveObjectSlotInfo* acc = &activeObjectSlots_[slot_id];
      
      ObjectConnectionState msg;
      msg.objectID = slot_id;
      msg.factoryID = acc->assignedFactoryID;
      msg.connected = acc->connected;
      msg.device_type = acc->device_type;
      RobotInterface::SendMessage(msg);
    }
    
    // Connect to active object
    bool HAL::AssignSlot(u32 slot_id, u32 factory_id)
    {
      if (slot_id >= MAX_NUM_ACTIVE_OBJECTS) {
        return false;
      }
      
      ActiveObjectSlotInfo* acc = &activeObjectSlots_[slot_id];
      
      if (factory_id != 0) {
        
        // If the slot is active send disconnect message if it's occupied by a different
        // device than the one requested. If it's the same as the one requested, send
        // a connect message.
        if (acc->connected) {
          if (acc->assignedFactoryID != factory_id) {
            acc->connected = false;
          }
          SendObjectConnectionState(slot_id);
        }
        
        // Queue connection to active object by setting factory id
        acc->assignedFactoryID = factory_id;
        
      } else {
        
        // Send disconnect response
        // NOTE: The id is that of the currently connected object if there was one.
        //       Otherwise the id is 0.
        acc->receiver->disable();
        acc->connected = false;
        SendObjectConnectionState(slot_id);
        acc->assignedFactoryID = 0;
        
      }
      
      return true;
    }
    
    int8_t CalculateObjectRSSI(double signalStrength)
    {
      // Convert the webots signal strength to similar values to what we get from the robot.
      // These numbers were just approximated by experimenting on webots
      return 200 / signalStrength + 40;
    }
    
    void ActiveObjectsUpdate(void)
    {

      // Check for blocks that should disconnect due to timeout
      for (u32 i = 0; i < MAX_NUM_ACTIVE_OBJECTS; ++i) {
        ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
        if (cubeInfo->connected && (HAL::GetTimeStamp() - cubeInfo->lastHeardTime > 3000)) {  // Threshold needs to be greater than ActiveBlock::DISCOVERED_MESSAGE_PERIOD * ActiveBlock::TIMESTEP
          cubeInfo->connected = false;
          SendObjectConnectionState(i);
        }
      }
      
      
      // Monitor discovery channel and forward on for blocks that are not connected.
      // For blocks that are connected, update lastHeardTime
      while (objectDiscoveryReceiver_->getQueueLength() > 0) {
        BlockMessages::LightCubeMessage lcm;
        memcpy(lcm.GetBuffer(), objectDiscoveryReceiver_->getData(), objectDiscoveryReceiver_->getDataSize());
        switch(lcm.tag)
        {
          case BlockMessages::LightCubeMessage::Tag_discovered:
          {
            ObjectDiscovered odMsg;
            memcpy(odMsg.GetBuffer(), lcm.discovered.GetBuffer(), lcm.discovered.Size());
            
            // If autoconnect is enabled, assign this block to a slot if another block
            // of the same type is not already assigned.
            if (autoConnectToBlocks_ && !IsSameTypeActiveObjectAssigned(odMsg.device_type)) {
              for (u32 i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
                ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
                if (cubeInfo->assignedFactoryID == 0) {
                  PRINT_NAMED_INFO("SIM", "sim_hal.Update.AutoAssignedObject: FactoryID 0x%x, type 0x%hx, slot %d",
                                   odMsg.factory_id, odMsg.device_type, i);
                  cubeInfo->assignedFactoryID = odMsg.factory_id;
                  break;
                }
              }
            }
            
            // Connect objects whose connections are pending
            // Update last heard time
            bool isConnected = false;
            for (u32 i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
              ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
              if (odMsg.factory_id == cubeInfo->assignedFactoryID) {
                
                if (!cubeInfo->connected) {
                  // The pending block connection is processed here so send an ObjectConnectionState message.
                  // Listen channel is the factoryID. Send channel is factoryID + 1.
                  cubeInfo->receiver->setChannel(cubeInfo->assignedFactoryID);
                  cubeInfo->receiver->enable(TIME_STEP);
                  cubeInfo->connected = true;
                  cubeInfo->device_type = odMsg.device_type;  // This is where we know the device type so set it here
                  SendObjectConnectionState(i);
                }
                
                cubeInfo->lastHeardTime = HAL::GetTimeStamp();
                isConnected = true;
                break;
              }
            }
            if (!isConnected) {
              // Block is not connected so send discovered message up to engine
              odMsg.rssi = CalculateObjectRSSI(objectDiscoveryReceiver_->getSignalStrength());
              RobotInterface::SendMessage(odMsg);
            }
            
            break;
          }
          default:
            AnkiWarn( 193, "sim_hal.ReadingDiscoveryChannel.UnexpectedMsg", 497, "Expected discovery tag but got %d", 1, lcm.tag);
            break;
        }
        
        objectDiscoveryReceiver_->nextPacket();
      }
      
      
      // Pass along block-moved messages to basestation
      // TODO: Make block comms receiver checking into a HAL function at some point
      //   and call it from the main execution loop
      for (u32 i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
        ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
        
        // If no block is yet connected at this slot go to next one
        if (!cubeInfo->connected) {
          continue;
        }
        
        while(cubeInfo->receiver->getQueueLength() > 0) {
          BlockMessages::LightCubeMessage lcm;
          memcpy(lcm.GetBuffer(), cubeInfo->receiver->getData(), cubeInfo->receiver->getDataSize());
          switch(lcm.tag)
          {
            case BlockMessages::LightCubeMessage::Tag_moved:
            {
              ObjectMoved m;
              memcpy(m.GetBuffer(), lcm.moved.GetBuffer(), lcm.moved.Size());
              m.objectID = i;
              m.robotID = 0;
              m.timestamp = HAL::GetTimeStamp();
              RobotInterface::SendMessage(m);
              break;
            }
            case BlockMessages::LightCubeMessage::Tag_stopped:
            {
              ObjectStoppedMoving m;
              memcpy(m.GetBuffer(), lcm.stopped.GetBuffer(), lcm.stopped.Size());
              m.objectID = i;
              m.robotID = 0;
              m.timestamp = HAL::GetTimeStamp();
              RobotInterface::SendMessage(m);
              break;
            }
            case BlockMessages::LightCubeMessage::Tag_tapped:
            {
              ObjectTapped m;
              memcpy(m.GetBuffer(), lcm.tapped.GetBuffer(), lcm.tapped.Size());
              m.objectID = i;
              m.robotID = 0;
              m.timestamp = HAL::GetTimeStamp();
              RobotInterface::SendMessage(m);
              break;
            }
            default:
            {
              printf("Received unexpected message from simulated object: %d\r\n", static_cast<int>(lcm.tag));
            }
          }
          cubeInfo->receiver->nextPacket();
        }
      }
      
    }
 
//    void HAL::ClearActiveObjectData(uint8_t slot) {
//      // TODO: If we ever start sending upAxis messages from sim robot, clear it here.
//    }
    
    void HAL::FacePrintf(const char *format, ...)
    {
      // Stub
    }
    
    void HAL::FaceUnPrintf()
    {
      // Stub
    }

    
    
  } // namespace Cozmo
} // namespace Anki
