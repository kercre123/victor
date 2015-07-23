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

#include "anki/cozmo/basestation/mat.h"

namespace Anki {
  
  namespace Cozmo {
    
    class Platform : public MatPiece
    {
    public:
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        static const Type LARGE_PLATFORM;
      };
      
      Platform(Type type);
      virtual MatPiece* CloneType() const override;
      
      virtual ObjectType GetType() const override { return _type; }
      
      virtual bool IsMoveable() const override { return true; }
      
    protected:
      
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
      const Type _type;
      
    }; // class Bridge
    
    
    inline MatPiece* Platform::CloneType() const
    {
      // Call the copy constructor
      return new Platform(_type);
    }
    
    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_PLATFORM_H