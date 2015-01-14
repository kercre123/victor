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

#include "json/json.h"

namespace Anki {
namespace Cozmo {

  // Forward declarations:
  class CozmoGameHostImpl;
  
  class CozmoGameHost
  {
  public:
    enum PlaybackMode {
      LIVE_SESSION_NO_RECORD = 0,
      RECORD_SESSION,
      PLAYBACK_SESSION
    };
    
    CozmoGameHost();
    ~CozmoGameHost();
    
    // TODO: Package up other stuff (like name?) into Robot/UiDevice identifiers. For now, just alias int.
    using AdvertisingRobot    = int;
    using AdvertisingUiDevice = int;
    
    bool Init(const Json::Value& config);
    
    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
  private:
    
    CozmoGameHostImpl* _impl;
    
  }; // class CozmoGameHost
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_HOST_H

