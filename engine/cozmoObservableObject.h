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

#include "engine/objectPoseConfirmer.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/vision/engine/observableObject.h"

#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

// Fwd decl
class VizManager;

// Aliases
using ActiveID = s32;  // TODO: Change this to u32 and use 0 as invalid
using FactoryID = std::string;

class ObservableObject : public Vision::ObservableObject, private Util::noncopyable
{
public:

  static const ActiveID InvalidActiveID = -1;
  static const FactoryID InvalidFactoryID;
  
  ObservableObject(ObjectFamily family, ObjectType type)
  : _family(family)
  , _type(type)
  {
    
  }
  
  virtual ObservableObject* CloneType() const override = 0;
  
  // Can only be called once and only before SetPose is called. Will assert otherwise,
  // since this indicates programmer error.
  void InitPose(const Pose3d& pose, PoseState poseState);
  
  // Override base class SetID to use unique ID for each type (base class has no concept of ObjectType)
  virtual void SetID() override;
  
  ObjectFamily  GetFamily()  const { return _family; }
  ObjectType    GetType()    const { return _type; }
  
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
  
  virtual bool IsActive()                     const override  { return false; }
  void         SetActiveID(ActiveID activeID);
  ActiveID     GetActiveID()                  const   { return _activeID; }
  void         SetFactoryID(FactoryID factoryID);
  FactoryID    GetFactoryID()                 const   { return _factoryID; }

  // Override in derived classes to allow them to exist co-located with robot
  virtual bool CanIntersectWithRobot()        const   { return false; }
  
  // Can we assume there is exactly one of these objects at a give time?
  virtual bool IsUnique()                     const   { return false; }

  // Get the distance within which we are allowed to localize to objects
  // (This will probably need to be updated with COZMO-9672)
  static f32 GetMaxLocalizationDistance_mm();
  
  // CTI SetIsMoving/IsMoving methods with TimeStamp_t are forwarded to cozmo methods and marked as
  // final to force child classes to use RobotTimeStamp_t
  virtual bool IsMoving(TimeStamp_t* t = (TimeStamp_t*)nullptr) const override final;
  virtual void SetIsMoving(bool isMoving, TimeStamp_t t) override final { SetIsMoving(isMoving, RobotTimeStamp_t{t}); }
  
  // These match the CTI ObservableObject (base class)'s default implementation
  virtual bool IsMoving(RobotTimeStamp_t* t) const { return false; }
  virtual void SetIsMoving(bool isMoving, RobotTimeStamp_t t) {}
  
protected:
  
  // Make SetPose protected and friend ObjectPoseConfirmer so only it can
  // update objects' poses
  virtual void SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState) override;
  using Vision::ObservableObject::SetPoseState;
  friend ObjectPoseConfirmer;
  
  ActiveID _activeID = -1;
  FactoryID _factoryID = "";
  
  ObjectFamily  _family = ObjectFamily::Unknown;
  ObjectType    _type   = ObjectType::UnknownObject;
  
  bool _poseHasBeenSet = false;
  
  VizManager* _vizManager = nullptr;
  
}; // class ObservableObject

#pragma mark -
#pragma mark Inlined Implementations

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject) const {
  return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject,
                                       const Point3f& distThreshold,
                                       const Radians& angleThreshold) const
{
  Point3f Tdiff;
  Radians angleDiff;
  return IsSameAs(otherObject, distThreshold, angleThreshold,
                  Tdiff, angleDiff);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void ObservableObject::SetActiveID(ActiveID activeID)
{
  if(!IsActive()) {
    PRINT_NAMED_WARNING("ObservableObject.SetActiveID.NotActive", "ID: %d", GetID().GetValue());
    return;
  }
  
  _activeID = activeID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
