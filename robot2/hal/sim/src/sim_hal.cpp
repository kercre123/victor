/**
 * File: sim_hal.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/10/2013
 * Author: Daniel Casner <daniel@anki.com>
 * Revised for Cozmo 2: 02/27/2017
 *
 * Description:
 *
 *   Simulated HAL implementation for Cozmo 2.0
 *
 *   This is an "abstract class" defining an interface to lower level
 *   hardware functionality like getting an image from a camera,
 *   setting motor speeds, etc.  This is all the Robot should
 *   need to know in order to talk to its underlying hardware.
 *
 *   To avoid C++ class overhead running on a robot's embedded hardware,
 *   this is implemented as a namespace instead of a fullblown class.
 *
 *   This just defines the interface; the implementation (e.g., Real vs.
 *   Simulated) is given by a corresponding .cpp file.  Which type of
 *   is used for a given project/executable is decided by which .cpp
 *   file gets compiled in.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


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
#include "clad/types/activeObjectConstants.h"

// Webots Includes
#include <webots/Robot.hpp>
#include <webots/Supervisor.hpp>
#include <webots/PositionSensor.hpp>
#include <webots/Emitter.hpp>
#include <webots/Motor.hpp>
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>
#include <webots/Display.hpp>
#include <webots/Gyro.hpp>
#include <webots/DistanceSensor.hpp>
#include <webots/Accelerometer.hpp>
#include <webots/Receiver.hpp>
#include <webots/Connector.hpp>
#include <webots/LED.hpp>

#define DEBUG_GRIPPER 0

// Enable this to light up the backpack whenever a sound is being played
// When this is on, HAL::SetLED() doesn't work.
#define LIGHT_BACKPACK_DURING_SOUND 0

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
      bool isGripperEnabled_ = false;

      // For pose information
      webots::GPS* gps_;
      webots::Compass* compass_;

      // IMU
      webots::Gyro* gyro_;
      webots::Accelerometer* accel_;

      // Prox sensors
      webots::DistanceSensor *proxCenter_;
      webots::DistanceSensor *cliffSensors_[HAL::CLIFF_COUNT];

      // Charge contact
      webots::Connector* chargeContact_;
      bool wasOnCharger_ = false;

      // Emitter / receiver for block communication
      webots::Receiver *objectDiscoveryReceiver_;
      webots::Emitter *blockCommsEmitter_;

      struct ActiveObjectSlotInfo {
        u32 assignedFactoryID;
        ObjectType object_type;
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

      // Audio
      // (Can't actually play sound in simulator, but proper handling of audio frames is still
      // necessary for proper animation timing)
      TimeStamp_t audioEndTime_ = 0;    // Expected end of audio
      u32 AUDIO_FRAME_TIME_MS = 33;     // Duration of single audio frame
      bool audioReadyForFrame_ = true;  // Whether or not ready to receive another audio frame
      

#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      //void GetGlobalPose(f32 &x, f32 &y, f32& rad);


      // Approximate open-loop conversion of wheel power to angular wheel speed
      float WheelPowerToAngSpeed(float power)
      {
        // Inverse of speed-power formula in WheelController
        float speed_mm_per_s = power / 0.004f;

        // Return linear speed m/s when usingTreads
        return -speed_mm_per_s / 1000.f;
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
        return power * 2*M_PI_F;
      }


      void MotorUpdate()
      {
        // Update position and speed info
        f32 posDelta = 0;
        for(int i = 0; i < MOTOR_COUNT; i++)
        {
          if (motors_[i]) {
            f32 pos = motorPosSensors_[i]->getValue();
            if (i == MOTOR_LEFT_WHEEL || i == MOTOR_RIGHT_WHEEL) {
              pos = motorPosSensors_[i]->getValue() * -1000.f;
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

      leftWheelMotor_  = webotRobot_.getMotor("LeftWheelMotor");
      rightWheelMotor_ = webotRobot_.getMotor("RightWheelMotor");

      headMotor_  = webotRobot_.getMotor("HeadMotor");
      liftMotor_  = webotRobot_.getMotor("LiftMotor");

      leftWheelPosSensor_ = webotRobot_.getPositionSensor("LeftWheelMotorPosSensor");
      rightWheelPosSensor_ = webotRobot_.getPositionSensor("RightWheelMotorPosSensor");

      headPosSensor_ = webotRobot_.getPositionSensor("HeadMotorPosSensor");
      liftPosSensor_ = webotRobot_.getPositionSensor("LiftMotorPosSensor");

      con_ = webotRobot_.getConnector("gripperConnector");


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

      // Gyro
      gyro_ = webotRobot_.getGyro("gyro");
      gyro_->enable(TIME_STEP);

      // Accelerometer
      accel_ = webotRobot_.getAccelerometer("accel");
      accel_->enable(TIME_STEP);

      // Proximity sensor
      proxCenter_ = webotRobot_.getDistanceSensor("forwardProxSensor");
      proxCenter_->enable(TIME_STEP);
      
      // Cliff sensors
      cliffSensors_[HAL::CLIFF_FL] = webotRobot_.getDistanceSensor("cliffSensorFL");
      cliffSensors_[HAL::CLIFF_FR] = webotRobot_.getDistanceSensor("cliffSensorFR");
      cliffSensors_[HAL::CLIFF_BL] = webotRobot_.getDistanceSensor("cliffSensorBL");
      cliffSensors_[HAL::CLIFF_BR] = webotRobot_.getDistanceSensor("cliffSensorBR");
      cliffSensors_[HAL::CLIFF_FL]->enable(TIME_STEP);
      cliffSensors_[HAL::CLIFF_FR]->enable(TIME_STEP);
      cliffSensors_[HAL::CLIFF_BL]->enable(TIME_STEP);
      cliffSensors_[HAL::CLIFF_BR]->enable(TIME_STEP);

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
          PRINT_NAMED_ERROR("simHAL.MotorSetPower.UndefinedType", "%d", motor);
          return;
      }
    }

    // Reset the internal position of the specified motor to 0
    void HAL::MotorResetPosition(MotorID motor)
    {
      if (motor >= MOTOR_COUNT) {
        PRINT_NAMED_ERROR("simHAL.MotorResetPosition.UndefinedType", "%d", motor);
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
          // if usingTreads, fall through to just returning motorSpeeds_ since
          // it is already stored in mm/s

        case MOTOR_LIFT:
        case MOTOR_HEAD:
          return motorSpeeds_[motor];

        default:
          PRINT_NAMED_ERROR("simHAL.MotorGetSpeed.UndefinedType", "%d", motor);
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
          // if usingTreads, fall through to just returning motorSpeeds_ since
          // it is already stored in mm

        case MOTOR_LIFT:
        case MOTOR_HEAD:
          return motorPositions_[motor];

        default:
          PRINT_NAMED_ERROR("simHAL.MotorGetPosition.UndefinedType", "%d", motor);
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
      PRINT_NAMED_DEBUG("simHAL.EngageGripper.Locked", "");
#     endif
    }

    void HAL::DisengageGripper()
    {
      con_->unlock();
      con_->disablePresence();
      isGripperEnabled_ = false;
#     if DEBUG_GRIPPER
      PRINT_NAMED_DEBUG("simHAL.DisengageGripper.Unocked", "");
#     endif

    }

    bool IsSameTypeActiveObjectAssigned(ObjectType object_type)
    {
      DEV_ASSERT((object_type != InvalidObject) &&
                 (object_type != UnknownObject),
                 "sim_hal.IsSameTypeActiveObjectAssigned.InvalidType");
      
      for (u32 i = 0; i < MAX_NUM_ACTIVE_OBJECTS; ++i) {
        if (activeObjectSlots_[i].assignedFactoryID != 0 && activeObjectSlots_[i].object_type == object_type) {
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
          idMsg.hwRevision = 0;
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
          PRINT_NAMED_DEBUG("simHAL.Step.OnCharger", "");
          wasOnCharger_ = true;
        } else if (!BatteryIsOnCharger() && wasOnCharger_) {
          PRINT_NAMED_DEBUG("simHAL.Step.OffCharger", "");
          wasOnCharger_ = false;
        }

        return RESULT_OK;
      }


    } // step()

    
    
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
        PRINT_NAMED_ERROR("simHAL.SetLED.UnhandledLED", "%d", led_id);
      }
      #endif
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
      for (u8 i = 0; i < DISPLAY_WIDTH; ++i) {
        for (u8 j = 0; j < DISPLAY_HEIGHT; ++j) {
          if ((faceFrame_[i] >> j) & 1) {
            face_->drawPixel(i, j);
          }
        }
      }
    }

    u32 HAL::GetID()
    {
      return robotID_;
    }
    
    u16 HAL::GetRawProxData()
    {
      const u16 val = static_cast<u16>( proxCenter_->getValue() );
      return val;
    }

    u16 HAL::GetRawCliffData(const CliffID cliff_id)
    {
      if (cliff_id == HAL::CLIFF_COUNT) {
        PRINT_NAMED_ERROR("simHAL.GetRawCliffData.InvalidCliffID", "");
        return static_cast<u16>(cliffSensors_[HAL::CLIFF_FL]->getMaxRange());
      }
      return static_cast<u16>(cliffSensors_[cliff_id]->getValue());
    }
    
    u16 HAL::GetCliffOffLevel(const CliffID cliff_id)
    {
      return 0;
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
    
    Result HAL::StreamObjectAccel(const u32 activeID, const bool enable)
    {
      if (activeID >= MAX_NUM_ACTIVE_OBJECTS) {
        return RESULT_FAIL;
      }
      
      BlockMessages::LightCubeMessage m;
      m.tag = BlockMessages::LightCubeMessage::Tag_streamObjectAccel;
      m.streamObjectAccel.objectID = activeID;
      m.streamObjectAccel.enable = enable;
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
      msg.object_type = acc->object_type;
      RobotInterface::SendMessage(msg);
    }
    
    // Connect to active object
    Result HAL::AssignSlot(u32 slot_id, u32 factory_id)
    {
      if (slot_id >= MAX_NUM_ACTIVE_OBJECTS) {
        return RESULT_FAIL;
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
      
      return RESULT_OK;
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
          case BlockMessages::LightCubeMessage::Tag_available:
          {
            ObjectAvailable oaMsg;
            memcpy(oaMsg.GetBuffer(), lcm.available.GetBuffer(), lcm.available.Size());
            
            // If autoconnect is enabled, assign this block to a slot if another block
            // of the same type is not already assigned.
            if (autoConnectToBlocks_ && !IsSameTypeActiveObjectAssigned(oaMsg.objectType)) {
              for (u32 i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
                ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
                if (cubeInfo->assignedFactoryID == 0) {
                  PRINT_NAMED_INFO("SIM", "sim_hal.Update.AutoAssignedObject: FactoryID 0x%x, type 0x%x, slot %d",
                                   oaMsg.factory_id, oaMsg.objectType, i);
                  cubeInfo->assignedFactoryID = oaMsg.factory_id;
                  break;
                }
              }
            }
            
            // Connect objects whose connections are pending
            // Update last heard time
            bool isConnected = false;
            for (u32 i=0; i< MAX_NUM_ACTIVE_OBJECTS; ++i) {
              ActiveObjectSlotInfo* cubeInfo = &activeObjectSlots_[i];
              if (oaMsg.factory_id == cubeInfo->assignedFactoryID) {
                
                if (!cubeInfo->connected) {
                  // The pending block connection is processed here so send an ObjectConnectionState message.
                  // Listen channel is the factoryID. Send channel is factoryID + 1.
                  cubeInfo->receiver->setChannel(cubeInfo->assignedFactoryID);
                  cubeInfo->receiver->enable(TIME_STEP);
                  cubeInfo->connected = true;
                  cubeInfo->object_type = oaMsg.objectType;  // This is where we know the device type so set it here
                  SendObjectConnectionState(i);
                }
                
                cubeInfo->lastHeardTime = HAL::GetTimeStamp();
                isConnected = true;
                break;
              }
            }
            if (!isConnected) {
              // Block is not connected so send ObjectAvailable message up to engine
              oaMsg.rssi = CalculateObjectRSSI(objectDiscoveryReceiver_->getSignalStrength());
              RobotInterface::SendMessage(oaMsg);
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
            case BlockMessages::LightCubeMessage::Tag_accel:
            {
              ObjectAccel m;
              memcpy(m.GetBuffer(), lcm.accel.GetBuffer(), lcm.accel.Size());
              m.objectID = i;
              m.timestamp = HAL::GetTimeStamp();
              RobotInterface::SendMessage(m);
              break;
            }
            case BlockMessages::LightCubeMessage::Tag_upAxisChanged:
            {
              ObjectUpAxisChanged m;
              memcpy(m.GetBuffer(), lcm.upAxisChanged.GetBuffer(), lcm.upAxisChanged.Size());
              m.objectID = i;
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

    u8 HAL::GetWatchdogResetCounter()
    {
      return 0; // Simulator never watchdogs
    }
    
  } // namespace Cozmo
} // namespace Anki
