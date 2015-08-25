#ifndef __Anki_Cozmo_FaceWorld_H__
#define __Anki_Cozmo_FaceWorld_H__

#include "anki/vision/basestation/trackedFace.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
  class FaceWorld
  {
  public:
    
    FaceWorld();
    
    Result UpdateFace(Vision::TrackedFace& face);
  
    const Vision::TrackedFace& GetFace(Vision::TrackedFace::ID_t faceID) const;
    
  private:
    
    std::map<Vision::TrackedFace::ID_t, Vision::TrackedFace> _knownFaces;
    
  }; // class FaceWorld
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_FaceWorld_H__