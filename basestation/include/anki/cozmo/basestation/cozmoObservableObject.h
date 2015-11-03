/**
 * File: cozmoObservableObject.h
 *
 * Author: Andrew Stein (andrew)
 * Created: ?/?/2015
 *
 *
 * Description: Extends Vision::ObservableObject to add some Cozmo-specific
 *              stuff, like object families and types.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_ObservableObject_H__
#define __Anki_Cozmo_ObservableObject_H__

#include "anki/vision/basestation/observableObject.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/activeObjectTypes.h"

namespace Anki {
namespace Cozmo {
  
  class ObservableObject : public Vision::ObservableObject
  {
  public:
    
    ObservableObject(ObjectFamily family, ObjectType type)
    : _family(family)
    , _type(type)
    {
      
    }
    
    virtual ObservableObject* CloneType() const = 0;
    
    ObjectFamily  GetFamily()  const { return _family; }
    ObjectType    GetType()    const { return _type; }
    
    ActiveIdentityState GetIdentityState() const { return _identityState; }
    
    // Overload base IsSameAs() to first compare type and family
    // (Note that we have to overload all if we overload one)
    bool IsSameAs(const ObservableObject& otherObject,
                  const Point3f& distThreshold,
                  const Radians& angleThreshold,
                  Point3f& Tdiff,
                  Radians& angleDiff) const;
    
    bool IsSameAs(const ObservableObject& otherObject) const;

    bool IsSameAs(const ObservableObject& otherObject,
                  const Point3f& distThreshold,
                  const Radians& angleThreshold) const;
    
    bool IsExistenceConfirmed() const;
    
  protected:
    
    ObjectFamily  _family = ObjectFamily::Unknown;
    ObjectType    _type   = ObjectType::Unknown;
    
    ActiveIdentityState _identityState = ActiveIdentityState::Unidentified;
    
  }; // class ObservableObject
  
#pragma mark -
#pragma mark Inlined Implementations
  
  inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         Point3f& Tdiff,
                                         Radians& angleDiff) const
  {
    // The two objects can't be the same if they aren't the same type!
    bool isSame = this->GetType() == otherObject.GetType() && this->GetFamily() == otherObject.GetFamily();
    
    if(isSame) {
      isSame = Vision::ObservableObject::IsSameAs(otherObject, distThreshold, angleThreshold, Tdiff, angleDiff);
    }
    
    return isSame;
  }
  
  inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject) const {
    return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance());
  }

  inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold) const
  {
    Point3f Tdiff;
    Radians angleDiff;
    return IsSameAs(otherObject, distThreshold, angleThreshold,
                    Tdiff, angleDiff);
  }
  
  inline bool ObservableObject::IsExistenceConfirmed() const {
    return ((!IsActive() || ActiveIdentityState::Identified == GetIdentityState()) &&
            GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT &&
            !IsPoseStateUnknown());
  }
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ObservableObject_H__