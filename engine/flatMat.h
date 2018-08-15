/**
 * File: flatMat.h
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Defines a basic flat mat piece, which is just a thin rectangular
 *              surface with markers for localization.
 *
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_FLATMAT_H
#define ANKI_COZMO_FLATMAT_H

#include "engine/mat.h"

namespace Anki {
  
  namespace Vector {
    
    class FlatMat : public MatPiece
    {
    public:

      using Type = ObjectType;
      
      FlatMat(Type type);
      
      virtual FlatMat* CloneType() const override;

      virtual bool IsMoveable() const override { return false; }
      
    protected:
      
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
      virtual void GeneratePreActionPoses(const PreActionPose::ActionType type,
                                          std::vector<PreActionPose>& preActionPoses) const override {};
    }; // class FlatMat
    
    
    inline FlatMat* FlatMat::CloneType() const
    {
      // Call the copy constructor
      return new FlatMat(_type);
    }
    
  } // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_FLATMAT_H
