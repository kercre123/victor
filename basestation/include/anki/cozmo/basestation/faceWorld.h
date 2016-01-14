#ifndef __Anki_Cozmo_FaceWorld_H__
#define __Anki_Cozmo_FaceWorld_H__

#include "anki/vision/basestation/trackedFace.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/actionTypes.h"

#include <map>
#include <vector>

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
    
    // Returns number of known faces
    // Actual face IDs returned in faceIDs
    std::vector<Vision::TrackedFace::ID_t> GetKnownFaceIDs() const;
    
    // Returns number of known faces observed since seenSinceTime_ms
    std::map<TimeStamp_t, Vision::TrackedFace::ID_t> GetKnownFaceIDsObservedSince(TimeStamp_t seenSinceTime_ms) const;

    // Returns time of the last observed face.
    // 0 if no face was ever observed.
    TimeStamp_t GetLastObservedFace(Pose3d& p) const;
    
  private:
    
    Robot& _robot;
    struct KnownFace {
      Vision::TrackedFace      face;
      VizManager::Handle_t     vizHandle;

      KnownFace(Vision::TrackedFace& faceIn);
    };
    
    std::map<Vision::TrackedFace::ID_t, KnownFace> _knownFaces;
    
    TimeStamp_t _deletionTimeout_ms = 4000;

    Vision::TrackedFace::ID_t _idCtr = 0;
    
    Pose3d      _lastObservedFacePose;
    TimeStamp_t _lastObservedFaceTimeStamp = 0;
    
    // The distance (in mm) threshold inside of which to head positions are considered to be the same face
    static constexpr float headCenterPointThreshold = 220.f;
    
    
    void RemoveFaceByID(Vision::TrackedFace::ID_t faceID);
    
  }; // class FaceWorld
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_FaceWorld_H__
