/*
 * File:          cozmoEngine.h
 * Date:          1/9/2015
 *
 * Description:   Example game class built around a CozmoEngine
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#ifndef ANKI_COZMO_GAME_HOST_H
#define ANKI_COZMO_GAME_HOST_H

#include "anki/common/types.h"
#include "json/json.h"

// TODO: Remove dependency on this
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

namespace Anki {
  
  // Forward declarations:
  namespace Vision {
    class Image;
  }
  
namespace Cozmo {

  // Forward declarations:
  class CozmoEngineHost;
  class CozmoGameImpl;
  class CozmoGameHostImpl;
  class CozmoGameClientImpl;
  
  // Common API whether host or client
  class CozmoGame
  {
  public:
    
    enum PlaybackMode {
      LIVE_SESSION_NO_RECORD = 0
      ,RECORD_SESSION
      ,PLAYBACK_SESSION
    };
    
    enum RunState {
      STOPPED = 0
      ,WAITING_FOR_UI_DEVICES
      ,WAITING_FOR_ROBOTS
      ,ENGINE_RUNNING
    };
    
    CozmoGame();
    virtual ~CozmoGame();
    
    virtual bool Init(const Json::Value& config) = 0;
    
    // Ticked with game heartbeat:
    virtual void Update(const float currentTime_sec) = 0;
    
    RunState GetRunState() const;
    
    //
    // Vision-related API
    //
    // TODO: Use shared memory / events to get rid of these
    
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
        
  protected:
    
    CozmoGameImpl* _impl;
    
  }; // class CozmoGame
  
  class CozmoGameHost : public CozmoGame
  {
  public:
    
    CozmoGameHost();
    ~CozmoGameHost();
    
    //
    // API Inherited from CozmoGame:
    //
    virtual bool Init(const Json::Value& config) override;
    
    virtual void Update(const float currentTime_sec) override;
    
    //
    // Host-specific API for establishing connections:
    //
    
    // TODO: Package up other stuff (like name?) into Robot/UiDevice identifiers. For now, just alias int.
    using AdvertisingRobot    = int;
    using AdvertisingUiDevice = int;

    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    //bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    //bool ConnectToRobot(AdvertisingRobot whichRobot);
    
  private:
    
    CozmoGameHostImpl* _hostImpl;
    
  }; // class CozmoGameHost
  
  
  class CozmoGameClient : public CozmoGame
  {
  public:
    
    CozmoGameClient();
    ~CozmoGameClient();
    
    //
    // API Inherited from CozmoGame:
    //
    virtual bool Init(const Json::Value& config) override;
    
    virtual void Update(const float currentTime_sec) override;
    
  private:
    
    CozmoGameClientImpl* _clientImpl;
    
  }; // class CozmoGameClient
  
  
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_HOST_H

