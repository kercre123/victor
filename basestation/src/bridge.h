/**
 * File: bridge.h
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Defines a Bridge piece, which is a type of mat (since it is 
 *              something the robot can drive on). A Bridge is a long, skinny
 *              mat with two entry points (one at either end) and "unsafe"
 *              regions on either side. It is not necessarily flat.
 *
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BRIDGE_H
#define ANKI_COZMO_BRIDGE_H

#include "anki/cozmo/basestation/mat.h"

namespace Anki {
  
  namespace Cozmo {
    
    class Bridge : public MatPiece
    {
    public:
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        static const Type LONG_BRIDGE;
        static const Type SHORT_BRIDGE;
      };
      
      Bridge(Type type);
      virtual Bridge* CloneType() const override;
      
      virtual ObjectType GetType() const override { return _type; }
      
      virtual bool IsMoveable() const override { return true; }
      
    protected:
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const override;
      
      const Type _type;
      
    }; // class Bridge
    
    
    inline Bridge* Bridge::CloneType() const
    {
      // Call the copy constructor
      return new Bridge(_type);
    }

    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_BRIDGE_H