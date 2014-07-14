/**
 * File: mat.cpp
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Implements a MatPiece object, which is a "mat" that Cozmo drives
 *              around on with VisionMarkers at known locations for localization.
 *
 *              MatPiece inherits from the generic Vision::ObservableObject.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/



#include "anki/cozmo/basestation/mat.h"

#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"


namespace Anki {
  namespace Cozmo {
    
    // MatPiece has no rotation ambiguities but we still need to define this
    // static const here to instatiate an empty list.
    const std::vector<RotationMatrix3d> MatPiece::rotationAmbiguities_;
    
    MatPiece::MatPiece(ObjectType_t type, bool isFirstPiece)
    : Vision::ObservableObject(type)
    {
      
      // TODO: Use a MatTypeLUT and MatDefinitions file, like we do with blocks
      
      //#include "anki/cozmo/basestation/Mat_AnkiLogoPlus8Bits_8x8.def"
#include "anki/cozmo/basestation/Mat_Letters_30mm_4x4.def"
     
      if(isFirstPiece) {
        // If this is the first mat piece, we want to use its pose as the world's
        // origin directly.
        pose_.SetParent(Pose3d::GetWorldOrigin());
      } else {
        // Add an origin to use as this mat piece's reference, until such time
        // that we want to make it relative to another mat piece or some
        // common origin
        pose_.SetParent(&Pose3d::AddOrigin());
      }
      
    };
    
    std::vector<RotationMatrix3d> const& MatPiece::GetRotationAmbiguities() const
    {
      return MatPiece::rotationAmbiguities_;
    }
    
    void MatPiece::Visualize() const
    {
      // TODO
    }
    
    Quad2f MatPiece::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      // TODO
    }
    
  } // namespace Cozmo
  
} // namespace Anki
