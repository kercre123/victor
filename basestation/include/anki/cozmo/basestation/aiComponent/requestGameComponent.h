/**
 * File: requestGameComponent.h
 *
 * Author: Kevin M. Karol
 * Created: 05/18/17
 *
 * Description: Component which keeps track of what game cozmo should request next
 * across all ways game requests can be initiated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_AIComponent_RequestGameComponent_H__
#define __Cozmo_Basestation_AIComponent_RequestGameComponent_H__

#include "anki/common/types.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "clad/types/unlockTypes.h"

#include "util/signals/simpleSignal_fwd.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class Robot;
  
struct GameRequestData{
  GameRequestData(UnlockId unlockID, int weight)
  : _unlockID(unlockID)
  , _weight(weight){}
  
  UnlockId   _unlockID;
  int _weight;
};
  
class RequestGameComponent{
public:
  RequestGameComponent(Robot& robot);
  ~RequestGameComponent() {};
  
  // Returns the unlockID of the next game type to request
  // returns count if no game should be requested
  UnlockId IdentifyNextGameTypeToRequest(const Robot& robot);
  
  
  // declaration for any event handler
  template<typename T>
  void HandleMessage(const T& msg);

protected:
  // RequestGame should be the only class that registers a game has been requested
  // when it's actually completed making the request so that the behavior isn't
  // penalized if it's interrupted before it makes the request
  friend class IBehaviorRequestGame;
  void RegisterRequestingGameType(UnlockId unlockID);
  
private:
  std::vector<Signal::SmartHandle> _eventHandles;
  
  std::map<UnlockId, GameRequestData> _gameRequests;
  std::map<UnlockId, GameRequestData> _defaultGameRequests;
  // track the last game that was requested so that we don't request the same
  // game over and over again
  UnlockId _lastGameRequested;
  
  // Keep track of when game says requests can/cant happen
  bool _canRequestGame;
  
  // struct for caching game request information so that randomness etc
  // doesn't result in different results being returned within the same tick
  struct CacheRequestData{
    UnlockId _cachedRequest;
    BaseStationTime_t _lastTimeUpdated_ns = 0;
  };
  
  CacheRequestData _cacheData;
  
  
  
};
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_AIComponent_RequestGameComponent_H__
