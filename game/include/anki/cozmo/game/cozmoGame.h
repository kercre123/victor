/*
 * File:          cozmoGame.h
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

#include "anki/cozmo/shared/cozmoTypes.h"

// TODO: Remove dependency on this
//#include "anki/cozmo/messageBuffers/robot/robotMessages.h"
#include "anki/cozmo/messageBuffers/game/UiMessagesG2U.h"

namespace Anki {
  
  // Forward declarations:
  namespace Vision {
    class Image;
  }
  
namespace Cozmo {

  // Forward declarations:
  class CozmoEngineHost;
  class CozmoGameImpl;
  
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
    ~CozmoGame();
    
    Result Init(const Json::Value& config);
    
    Result StartEngine(Json::Value config);
    
    // Ticked with game heartbeat:
    Result Update(const float currentTime_sec);
    
    RunState GetRunState() const;
    
    // TODO: Package up other stuff (like name?) into Robot/UiDevice identifiers. For now, just alias int.
    using AdvertisingRobot    = int;
    using AdvertisingUiDevice = int;
    
    // For adding a real robot to the list of availale ones advertising, using its
    // known IP address. This is only necessary until we have real advertising
    // capability on real robots.
    // TODO: Remove this once we have sorted out the advertising process for real robots
    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    // Return number of robots connected
    int GetNumRobots() const;
    
    //
    // Vision-related API
    //
    // TODO: Use shared memory / events to get rid of these
    
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    // Get the list of vision markers seen by the device camera on the last call
    // to ProcessDeviceImage().
    // NOTE: this will only be useful if the device vision processor is running
    //  synchronously!
    // TODO: Remove in favor of sending these messages to the UI
    //  (For now, this doesn't work on a client device because its game can't
    //   talk straight to the UI on the device it is running on)
    const std::vector<G2U::DeviceDetectedVisionMarker>& GetVisionMarkersDetectedByDevice() const;
    
    void SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode);
    
  protected:
    
    CozmoGameImpl* _impl;
    
  }; // class CozmoGame
  
  
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_HOST_H

