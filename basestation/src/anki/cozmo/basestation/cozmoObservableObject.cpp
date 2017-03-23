/**
 * File: cozmoObservableObject.pp
 *
 * Author: Andrew Stein
 * Date:   2/21/2017
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "anki/cozmo/basestation/cozmoObservableObject.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
// Only localize to / identify active objects within this distance
CONSOLE_VAR_RANGED(f32, kMaxLocalizationDistance_mm, "PoseConfirmation", 250.f, 50.f, 1000.f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
f32 ObservableObject::GetMaxLocalizationDistance_mm()
{
  return kMaxLocalizationDistance_mm;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObservableObject::SetID()
{
  if(IsUnique())
  {
    static std::map<ObjectType, ObjectID> typeToUniqueID;
    
    const ObjectType objType = GetType();
    
    auto iter = typeToUniqueID.find(objType);
    if(iter == typeToUniqueID.end())
    {
      // First instance with this type. Add new entry.
      Vision::ObservableObject::SetID();
      const ObjectID& newID = GetID();
      typeToUniqueID.emplace(objType, newID);
    }
    else
    {
      // Use existing ID for this type
      _ID = iter->second;
    }
  }
  else
  {
    Vision::ObservableObject::SetID();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObservableObject::InitPose(const Pose3d& pose, PoseState poseState)
{
  // This indicates programmer error: InitPose should only be called once on
  // an object and never once SetPose has been called
  DEV_ASSERT_MSG(!_poseHasBeenSet,
                 "ObservableObject.InitPose.PoseAlreadySet",
                 "%s Object %d",
                 EnumToString(GetType()), GetID().GetValue());
  
  SetPose(pose, -1.f, poseState);
  _poseHasBeenSet = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObservableObject::SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState)
{
  Vision::ObservableObject::SetPose(newPose, fromDistance, newPoseState);
  _poseHasBeenSet = true; // Make sure InitPose can't be called after this
  
  // Every object's pose should always be able to find a path to a valid origin without crashing
  if(ANKI_DEV_CHEATS) {
    const Pose3d* parent = GetPose().GetParent();
    ANKI_VERIFY(GetPose().FindOrigin().IsOrigin(), "ObservableObject.SetPose.UnableToFindOrigin",
                "%s %s ID:%d at %s with parent '%s'(%p)",
                EnumToString(GetFamily()), EnumToString(GetType()), GetID().GetValue(),
                GetPose().GetTranslation().ToString().c_str(),
                parent == nullptr ? "(none)" : parent->GetName().c_str(), parent);
  }
}
  
} // namespace Cozmo
} // namespace Anki
