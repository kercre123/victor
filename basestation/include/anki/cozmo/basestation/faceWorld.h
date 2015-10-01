#ifndef __Anki_Cozmo_FaceWorld_H__
#define __Anki_Cozmo_FaceWorld_H__

#include "anki/vision/basestation/trackedFace.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration:
  class Robot;
  
  class FaceWorld
  {
  public:
    
    FaceWorld(Robot& robot);
    
    Result Update();
    Result AddOrUpdateFace(Vision::TrackedFace& face);
  
    // Returns nullptr if not found
    const Vision::TrackedFace* GetFace(Vision::TrackedFace::ID_t faceID) const;
    
  private:
    
    Robot& _robot;
    struct KnownFace {
      Vision::TrackedFace      face;
      VizManager::Handle_t     vizHandle;

      KnownFace(Vision::TrackedFace& faceIn);
    };
    
    std::map<Vision::TrackedFace::ID_t, KnownFace> _knownFaces;
    
    TimeStamp_t _deletionTimeout_ms = 3000;
    
    Result UpdateFaceTracking(const Vision::TrackedFace& face);
    
    Vision::TrackedFace::ID_t _idCtr = 0;
    
  }; // class FaceWorld
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_FaceWorld_H__