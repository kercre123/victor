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

#include "anki/cozmo/basestation/events/BaseStationEvent.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {

  // Forward declarations:
  class CozmoEngineHost;
  
  class CozmoGameHost : public IBaseStationEventListener
  {
  public:
    enum PlaybackMode {
      LIVE_SESSION_NO_RECORD = 0,
      RECORD_SESSION,
      PLAYBACK_SESSION
    };
    
    CozmoGameHost();
    ~CozmoGameHost();
    
    void Init(const Json::Value& config);
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
    // Process raised events from CozmoEngine.
    // Req'd by IBaseStationEventListener
    virtual void OnEventRaised( const IBaseStationEventInterface* event ) override;
    
  private:
    CozmoEngineHost *cozmoEngine_;
    
  };
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_HOST_H

