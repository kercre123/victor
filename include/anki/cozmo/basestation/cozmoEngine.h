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

#include "anki/vision/basestation/image.h"

#include "json/json.h"

namespace Anki {
  
  namespace Comms {
    class AdvertisementService;
    class IComms;
  }
  
namespace Cozmo {

  // Forward declarations:
  class VisionProcessingThread;
  class IMessageHandler;
  class MultiClientComms;
  
  class CozmoEngine
  {
  public:
    
    CozmoEngine();
    ~CozmoEngine();
    
    virtual bool IsHost() const = 0;
    
    virtual Result Init(const Json::Value& config);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    virtual Result Update();
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
  protected:
    bool _isInitialized;
    
    VisionProcessingThread* _robotVisionThread;
    VisionProcessingThread* _deviceVisionThread;
    
    MultiClientComms* _robotComms;
    MultiClientComms* _gameComms;
    
    IMessageHandler* _robotVisionMsgHandler;
    IMessageHandler* _deviceVisionMsgHandler;
    
  }; // class CozmoEngine
  
  
  // TODO: Move derived classes to their own files
  
  // Forward declarations:
  class BasestationMain;
  
  class CozmoEngineHost : public CozmoEngine
  {
  public:
    CozmoEngineHost();
    ~CozmoEngineHost();
    
    virtual bool IsHost() const override { return true; }
    
    virtual Result Update() override;
    
  protected:
    BasestationMain* _basestation;
    
    Comms::AdvertisementService* _robotAdvertisementService;
    Comms::AdvertisementService* _uiAdvertisementService;
    
  }; // class CozmoEngineHost
  
  
  class CozmoEngineClient : public CozmoEngine
  {
  public:
    
    CozmoEngineClient();
    
    virtual bool IsHost() const override { return false; }
    
    void SetHostIP(const char ipAddress);
    
  }; // class CozmoEngineClient
  
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
