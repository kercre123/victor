/*
 * File:          cozmoEngine.h
 * Date:          12/23/2014
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo on a device. The base class has all the pieces
 *                needed by a device functioning as either host or client. The derived
 *                classes add host- or client-specific functionality.
 *
 *                 - Device Vision Processor (for processing images from the host device's camera)
 *                 - Robot Vision Processor (for processing images from a physical robot's camera)
 *                 - RobotComms
 *                 - GameComms and GameMsgHandler
 *                 - RobotVisionMsgHandler
 *                   - Uses Robot Comms to receive image msgs from robot.
 *                   - Passes images onto Robot Vision Processor
 *                   - Sends processed image markers to the basestation's port on
 *                     which it receives messages from the robot that sent the image.
 *                     In this way, the processed image markers appear to come directly
 *                     from the robot to the basestation.
 *                   - While we only have TCP support on robot, RobotVisionMsgHandler 
 *                     will also forward non-image messages from the robot on to the basestation.
 *                 - DeviceVisionMsgHandler
 *                   - Look into mailbox that Device Vision Processor dumps results 
 *                     into and sends them off to basestation over GameComms.
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#ifndef ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
#define ANKI_COZMO_BASESTATION_COZMO_ENGINE_H

#include "anki/cozmo/basestation/messages.h"

#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/cameraCalibration.h"

// TODO: Remove dependence on this include
#include "anki/cozmo/basestation/basestation.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
  class CozmoEngineImpl;
  class CozmoEngineHostImpl;
  class CozmoEngineClientImpl;
  
  class CozmoEngine
  {
  public:
    
    CozmoEngine();
    virtual ~CozmoEngine();
    
    virtual bool IsHost() const = 0;
    
    Result Init(const Json::Value& config,
                const Vision::CameraCalibration& deviceCamCalib);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    using Time = unsigned long long int;
    Result Update(const Time currTime_sec);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    bool WasLastDeviceImageProcessed();
    
    // TODO: Package up other stuff (like name?) into Robot/UiDevice identifiers. For now, just alias int.
    using AdvertisingRobot = int;
    using AdvertisingUiDevice = int;
    
    // Get list of available robots / UI devices
    void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);

    
    // Request a connection to a specific robot / UI device from the list returned above.
    // Returns true on successful connection, false otherwise.
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot); // virtual so Host can do something different for force-added robots
    
    // TODO: Add IsConnected methods
    /*
    // Check to see if a specified robot / UI device is connected
    bool IsRobotConnected(AdvertisingRobot whichRobot) const;
    bool IsUiDeviceConnected(AdvertisingUiDevice whichDevice) const;
    */
    
    // TODO: Remove these in favor of it being handled via messages instead of direct API polling
    bool CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg);
    
    virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) = 0;
    virtual bool GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations) = 0;
    
  protected:
    
    // This will just point at either the host or client impl pointer in a
    // derived class
    CozmoEngineImpl* _impl;
    
  }; // class CozmoEngine
  
  
  // TODO: Move derived classes to their own files
  
  
  class CozmoEngineHost : public CozmoEngine
  {
  public:
    
    CozmoEngineHost();
    
    virtual bool IsHost() const override { return true; }
    
    void GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices);
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    
    void ForceAddRobot(AdvertisingRobot robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    // TODO: Add ability to playback/record
    
    // Overload to specially handle robot added by ForceAddRobot
    // TODO: Remove once we no longer need forced adds
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;
    
    // TODO: Remove these in favor of it being handled via messages instead of direct API polling
    // TODO: Or promote to base class when we pull robots' visionProcessingThreads out of basestation and distribute across devices
    virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) override;
    virtual bool GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations) override;
    
  protected:
    CozmoEngineHostImpl* _hostImpl;
    
  }; // class CozmoEngineHost
  
  
  class CozmoEngineClient : public CozmoEngine
  {
  public:
    
    CozmoEngineClient();
    
    virtual bool IsHost() const override { return false; }
    
    virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) override;
    virtual bool GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations) override;
    
  protected:
    CozmoEngineClientImpl* _clientImpl;
    
  }; // class CozmoEngineClient
  
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
