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
    static const s32 MinTimesToSeeFace = 4;
    
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
    TimeStamp_t GetLastObservedFace(Pose3d& p);
    
  private:
    
    Robot& _robot;
    struct KnownFace {
      Vision::TrackedFace      face;
      VizManager::Handle_t     vizHandle;
      s32                      numTimesObserved = 0;

      KnownFace(Vision::TrackedFace& faceIn);
    };
    
    std::map<Vision::TrackedFace::ID_t, KnownFace> _knownFaces;
    using KnownFaceIter = std::map<Vision::TrackedFace::ID_t, KnownFace>::iterator;
    
    TimeStamp_t _deletionTimeout_ms = 30000;

    //Vision::TrackedFace::ID_t _idCtr = 0;
    
    Pose3d      _lastObservedFacePose;
    TimeStamp_t _lastObservedFaceTimeStamp = 0;
    
    
    void RemoveFaceByID(Vision::TrackedFace::ID_t faceID);
    void RemoveFace(KnownFaceIter& faceIter);
    
  }; // class FaceWorld
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_FaceWorld_H__
