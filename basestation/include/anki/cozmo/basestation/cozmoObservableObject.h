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

#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/vision/basestation/observableObject.h"

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
    
    // Can only be called once and only before SetPose is called. Will assert otherwise,
    // since this indicates programmer error.
    void InitPose(const Pose3d& pose, PoseState poseState);
    
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
    
    void SetVizManager(VizManager* vizManager) { _vizManager = vizManager; }
    
    void         SetActiveID(ActiveID activeID);
    ActiveID     GetActiveID()                  const   { return _activeID; }
    virtual bool IsActive()                     const   { return false; }
    void         SetFactoryID(FactoryID factoryID);
    FactoryID    GetFactoryID()                 const   { return _factoryID; }

    // Override in derived classes to allow them to exist co-located with robot
    virtual bool CanIntersectWithRobot() const { return false; }
    
  protected:
    
    // Make SetPose and SetPoseParent protected and friend ObjectPoseConfirmer so only it can
    // update objects' poses
    void SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState);
    using Vision::ObservableObject::SetPoseParent;
    using Vision::ObservableObject::SetPoseState;
    friend ObjectPoseConfirmer;
    
    ActiveID _activeID = -1;
    FactoryID _factoryID = 0;
    
    ObjectFamily  _family = ObjectFamily::Unknown;
    ObjectType    _type   = ObjectType::Unknown;
    
    ActiveIdentityState _identityState = ActiveIdentityState::Unidentified;
    
    bool _poseHasBeenSet = false;
    
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

  inline void ObservableObject::InitPose(const Pose3d& pose, PoseState poseState)
  {
    // This indicates programmer error: InitPose should only be called once on
    // an object and never once SetPose has been called
    ASSERT_NAMED_EVENT(!_poseHasBeenSet,
                       "ObservableObject.InitPose.PoseAlreadySet",
                       "%s Object %d",
                       EnumToString(GetType()), GetID().GetValue());
    
    SetPose(pose, -1.f, poseState);
    _poseHasBeenSet = true;
  }
  
  inline void ObservableObject::SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState)
  {
    Vision::ObservableObject::SetPose(newPose, fromDistance, newPoseState);
    _poseHasBeenSet = true; // Make sure InitPose can't be called after this
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
  
  inline void ObservableObject::SetActiveID(ActiveID activeID)
  {
    if(!IsActive()) {
      PRINT_NAMED_WARNING("ObservableObject.SetActiveID.NotActive", "ID: %d", GetID().GetValue());
      return;
    }
    
    _activeID = activeID;
    
    if (_activeID >= 0) {
      _identityState = ActiveIdentityState::Identified;
    }
  }
  
  inline void ObservableObject::SetFactoryID(FactoryID factoryID)
  {
    if(!IsActive()) {
      PRINT_NAMED_WARNING("ObservableObject.SetFactoryID.NotActive", "ID: %d", GetID().GetValue());
      return;
    }
    
    _factoryID = factoryID;
  }
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ObservableObject_H__
