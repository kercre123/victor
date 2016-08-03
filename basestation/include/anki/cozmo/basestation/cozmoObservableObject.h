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
#include "clad/types/activeObjectTypes.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {
  
  class VizManager;

  
  using ActiveID = s32;  // TODO: Change this to u32 and use 0 as invalid
  using FactoryID = u32;

  using FactoryIDArray = std::array<FactoryID, (size_t)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS>;

  class ObservableObject : public Vision::ObservableObject, private Util::noncopyable
  {
  public:

    static const ActiveID InvalidActiveID = -1;
    static const FactoryID InvalidFactoryID = 0;
    
    ObservableObject(ObjectFamily family, ObjectType type)
    : _family(family)
    , _type(type)
    {
      
    }
    
    virtual ObservableObject* CloneType() const = 0;
    
    ObjectFamily  GetFamily()  const { return _family; }
    ObjectType    GetType()    const { return _type; }
    
    // Returns Identified for non-Active objects and the active identity state
    // for Active objects.
    ActiveIdentityState GetIdentityState() const;
    
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
    
    void SetVizManager(VizManager* vizManager) { _vizManager = vizManager; }
    
    void         SetActiveID(ActiveID activeID)         { assert(IsActive());
                                                          _activeID = activeID;
                                                          if (_activeID >= 0) {
                                                            _identityState = ActiveIdentityState::Identified;
                                                          }
                                                        }
    ActiveID     GetActiveID()                  const   { return _activeID; }
    virtual bool IsActive()                     const   { return false; }
    void         SetFactoryID(FactoryID factoryID)      { assert(IsActive()); _factoryID = factoryID; }
    FactoryID    GetFactoryID()                 const   { return _factoryID; }

    
  protected:
    
    ActiveID _activeID = -1;
    FactoryID _factoryID = 0;
    
    ObjectFamily  _family = ObjectFamily::Unknown;
    ObjectType    _type   = ObjectType::Unknown;
    
    ActiveIdentityState _identityState = ActiveIdentityState::Unidentified;
    
    VizManager* _vizManager = nullptr;
    
  }; // class ObservableObject
  
  
  inline ActiveIdentityState ObservableObject::GetIdentityState() const {
    if(IsActive()) {
      return _identityState;
    } else {
      // Non-Active Objects are always "identified"
      return ActiveIdentityState::Identified;
    }
  }

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
    return ( //(!IsActive() || ActiveIdentityState::Identified == GetIdentityState()) &&
            GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT &&
            !IsPoseStateUnknown());
  }
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ObservableObject_H__
