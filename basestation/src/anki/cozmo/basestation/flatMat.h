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

#include "anki/cozmo/basestation/mat.h"

namespace Anki {
  
  namespace Cozmo {
    
    class FlatMat : public MatPiece
    {
    public:

      using Type = ObjectType;
      
      FlatMat(Type type);
      
      virtual FlatMat* CloneType() const override;
      
    protected:
      
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
    }; // class FlatMat
    
    
    inline FlatMat* FlatMat::CloneType() const
    {
      // Call the copy constructor
      return new FlatMat(_type);
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_FLATMAT_H