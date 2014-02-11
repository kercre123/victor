#include "anki/cozmo/basestation/mat.h"

namespace Anki {
  namespace Cozmo {
    
    // MatPiece has no rotation ambiguities but we still need to define this
    // static const here to instatiate an empty list.
    const std::vector<RotationMatrix3d> MatPiece::rotationAmbiguities_;
    
    std::vector<RotationMatrix3d> const& MatPiece::GetRotationAmbiguities() const
    {
      return MatPiece::rotationAmbiguities_;
    }
    
  } // namespace Cozmo
  
} // namespace Anki