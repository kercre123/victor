/**
 * File: poseOriginList.h
 *
 * Author: Andrew Stein
 * Created: 7/28/2016
 *
 *
 * Description: Defines a PoseOriginList for maintaining multiple origins (a.k.a. coordinate
 *              frames).
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Coretech_Common_PoseOriginList_H__
#define __Anki_Coretech_Common_PoseOriginList_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/poseOrigin.h"

#include "util/helpers/templateHelpers.h"
#include "util/math/numericCast.h"

#include <vector>
#include <map>

namespace Anki {
  
class PoseOriginList
{
public:
  static const PoseOriginID_t UnknownOriginID;
  
  PoseOriginList();
  ~PoseOriginList();
  
  // Add a new origin to the list and return its unique ID. This is the preferred way to add origins.
  PoseOriginID_t AddNewOrigin();
  
  // Allows you to add an origin with a specific ID, e.g. if you are mirroring another
  // origin lists via messages. Will refuse to add duplicate ID (and return false to indicate failure).
  // Use with care!
  bool AddOriginWithID(PoseOriginID_t ID);
  
  bool ContainsOriginID(PoseOriginID_t ID) const;
  
  const PoseOrigin& GetOriginByID(PoseOriginID_t ID) const;
  
  const PoseOrigin& GetCurrentOrigin()   const;
  PoseOriginID_t    GetCurrentOriginID() const;
  
  bool IsPoseInCurrentOrigin(const Pose3d& pose) const { return pose.HasSameRootAs(GetCurrentOrigin()); }
  
  size_t GetSize() const { return _origins.size(); }
  
  // Rejigger current origin to newOrigin, using the given transform.
  // Fail if newOrigin is not in the PoseOriginList.
  Result Rejigger(const PoseOrigin& newOrigin, const Transform3d& transform);
  
  void Flatten(PoseOriginID_t wrtOrigin);
  
  // ANKI_VERIFY that each origin is still "owned". Return false if not.
  bool SanityCheckOwnership() const;
  
private:
  
  // Make sure PoseOriginID_t and PoseID_t always match
  static_assert(std::is_same<PoseID_t, PoseOriginID_t>::value,
                "PoseOriginID_t and PoseID_t should be the same underlying type");
  
  PoseOriginID_t _nextID = UnknownOriginID + 1;
  
  std::map<PoseOriginID_t, std::unique_ptr<PoseOrigin>> _origins;
  
  struct
  {
    PoseOriginID_t    originID;
    const PoseOrigin* originPtr;
  } _current;
  
}; // class PoseOriginList
  
  
//
// Inlined methods:
//
inline const PoseOrigin& PoseOriginList::GetCurrentOrigin() const
{
  DEV_ASSERT(_current.originID != UnknownOriginID, "PoseOriginList.GetCurrentOrigin.NoCurrentOriginID");
  DEV_ASSERT(_current.originPtr != nullptr, "PoseOriginList.GetCurrentOrigin.NullCurrentOrigin");
  return *_current.originPtr;
}

inline PoseOriginID_t PoseOriginList::GetCurrentOriginID() const
{
  DEV_ASSERT_MSG(_current.originPtr == nullptr ||
                 _current.originPtr->GetID() == _current.originID,
                 "PoseOriginList.GetCurrentOriginID.CurrentIDMismatch",
                 "Origin.GetID:%d OriginID:%d",
                 _current.originPtr->GetID(), _current.originID);
  return _current.originID;
}
  
} // namespace Anki

#endif // __Anki_Coretech_Common_PoseOriginList_H__
