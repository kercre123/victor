/**
 * File: platform.h
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Defines a Platform piece, which is a type of mat (since it is
 *              something the robot can drive on). A Platform is a big "box"
 *              that a robot can drive around on top of.
 *
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_PLATFORM_H
#define ANKI_COZMO_PLATFORM_H

#include "engine/mat.h"

namespace Anki {
  
  namespace Vector {
    
    class Platform : public MatPiece
    {
    public:
      
      using Type = ObjectType;
      
      Platform(Type type);
      virtual Platform* CloneType() const override;
      
      virtual bool IsMoveable() const override { return true; }
      
    protected:
      
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
    }; // class Bridge
    
    
    inline Platform* Platform::CloneType() const
    {
      // Call the copy constructor
      return new Platform(_type);
    }
    
    
  } // namespace Vector
  
} // namespace Anki

#endif // ANKI_COZMO_PLATFORM_H
