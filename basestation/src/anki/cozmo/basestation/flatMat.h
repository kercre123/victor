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
      // TODO: Use a MatDefinitions file, like with blocks
      class Type : public ObjectType
      {
      protected:
        Type(const std::string& name) : ObjectType(name) { }
      public:
        // Define new mat piece types here, as static const Type:
        // (Note: don't forget to instantiate each in the .cpp file)
        static const Type LETTERS_4x4;
        static const Type ANKI_LOGO_8BIT;
        static const Type LAVA_PLAYTEST;
        static const Type GEARS_4x4;
      };
      
      FlatMat(Type type);
      
      virtual FlatMat* CloneType() const override;
      
      virtual ObjectType GetType() const override { return _type; }
      
      virtual bool IsMoveable() const override { return false; }
      
    protected:
      
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
      Type _type;
      
    }; // class FlatMat
    
    
    inline FlatMat* FlatMat::CloneType() const
    {
      // Call the copy constructor
      return new FlatMat(_type);
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_FLATMAT_H