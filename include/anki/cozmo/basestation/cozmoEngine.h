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
    
    Result Init(const Json::Value& config);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    using Time = unsigned long long int;
    Result Update(const Time currTime_sec);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    // TODO: Package up other stuff (like name?) into Robot/UiDevice identifiers. For now, just alias int.
    using AdvertisingRobot = int;
    using AdvertisingUiDevice = int;
    
    // Get list of available robots / UI devices
    void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);
    void GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices);
    
    // Request a connection to a specific robot / UI device from the list returned above.
    // Returns true on successful connection, false otherwise.
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    
    // TODO: Add IsConnected methods
    /*
    // Check to see if a specified robot / UI device is connected
    bool IsRobotConnected(AdvertisingRobot whichRobot) const;
    bool IsUiDeviceConnected(AdvertisingUiDevice whichDevice) const;
    */
    
    // TODO: Remove this in favor of it being handled via messages instead of direct API polling
    bool CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg);
    
  protected:
    CozmoEngineImpl* _impl;
    
  }; // class CozmoEngine
  
  
  // TODO: Move derived classes to their own files
  
  
  class CozmoEngineHost : public CozmoEngine
  {
  public:
    CozmoEngineHost();
    
    virtual bool IsHost() const override { return true; }
    
    
  protected:
    CozmoEngineHostImpl* GetHostImpl();
    
  }; // class CozmoEngineHost
  
  
  class CozmoEngineClient : public CozmoEngine
  {
  public:
    
    CozmoEngineClient();
    
    virtual bool IsHost() const override { return false; }
    
  protected:
    CozmoEngineClientImpl* GetClientImpl();
    
  }; // class CozmoEngineClient
  
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
