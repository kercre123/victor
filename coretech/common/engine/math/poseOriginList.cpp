/**
 * File: poseOriginList.cpp
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

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/poseOrigin.h"
#include "coretech/common/engine/math/poseOriginList.h"

namespace Anki
{

// Static initialization
const PoseOriginID_t PoseOriginList::UnknownOriginID = 0;

PoseOriginList::PoseOriginList()
: _current{.originID = UnknownOriginID, .originPtr = nullptr}
{

}

PoseOriginList::~PoseOriginList()
{
  PRINT_NAMED_INFO("poseoriginlist.destructor", "%zu", GetSize());

  // Ignore whether unowned parents are allowed so we can delete
  // pose origins without worrying about the ordering here (since
  // an origin that's a parent of another "origin", thanks to rejiggering
  // might get deleted before its child)
  const bool wereAllowed = Pose3d::AreUnownedParentsAllowed();
  Pose3d::AllowUnownedParents(true);
  for(auto & origin : _origins)
  {
    origin.second.reset();
  }
  Pose3d::AllowUnownedParents(wereAllowed);
}

bool PoseOriginList::ContainsOriginID(PoseOriginID_t ID) const
{
  auto iter = _origins.find(ID);
  return (iter != _origins.end());
}

PoseOriginID_t PoseOriginList::AddNewOrigin()
{
  PoseOriginID_t ID = _nextID;

  const bool success = AddOriginWithID(ID);
  ANKI_VERIFY(success, "PoseOriginList.AddNewOrigin.AddWithIDFailed", "ID:%d", ID);
# pragma unused(success) // avoid unused var warnings in release/shipping

  return ID;
}

bool PoseOriginList::AddOriginWithID(PoseOriginID_t ID)
{
  const std::string name("Origin" + std::to_string(ID));
  auto result = _origins.emplace(ID, std::unique_ptr<PoseOrigin>(new PoseOrigin(name)));
  if(!ANKI_VERIFY(result.second, "PoseOriginList.AddOriginWithID.DuplicateID", "%d", ID))
  {
    if(ANKI_DEV_CHEATS)
    {
      Util::sAbort();
    }
    return false;
  }

  result.first->second->SetID(ID);
  _current.originID  = ID;
  _current.originPtr = result.first->second.get();
  _nextID = std::max(ID + 1, _nextID);

  return true;
}

const PoseOrigin& PoseOriginList::GetOriginByID(PoseOriginID_t ID) const
{
  auto iter = _origins.find(ID);
  if(!ANKI_VERIFY(iter != _origins.end(), "PoseOriginList.GetOriginByID.BadID", "%d", ID))
  {
    static const PoseOrigin DefaultOrigin("OriginListDefaultOrigin");
    return DefaultOrigin;
  }

  DEV_ASSERT_MSG(nullptr != iter->second, "PoseOriginList.GetOriginByID.NullOrigin", "%d", ID);

  return *iter->second;
}

Result PoseOriginList::Rejigger(const PoseOrigin& newOrigin, const Transform3d& transform)
{
  const PoseOriginID_t newOriginID = newOrigin.GetID();
  if(!ANKI_VERIFY(ContainsOriginID(newOriginID), "PoseOriginList.Rejigger.UnknownOrigin",
                  "Pose ID:%d (%s) not known to PoseOriginList", newOriginID, newOrigin.GetName().c_str()))
  {
    return RESULT_FAIL;
  }

  if(!ANKI_VERIFY(_current.originID != newOriginID, "PoseOriginList.Rejigger.NewOriginIsCurrent", "ID:%d", newOriginID))
  {
    return RESULT_FAIL;
  }

  auto origIter = _origins.find(_current.originID);
  auto newIter  = _origins.find(newOriginID);

  DEV_ASSERT_MSG(origIter != _origins.end(),
                 "PoseOriginList.Rejigger.InvalidCurrentOriginID", "ID:%d", _current.originID);
  DEV_ASSERT_MSG(newIter != _origins.end(),
                 "PoseOriginList.Rejigger.InvalidNewOriginID", "ID:%d", newOriginID);
  DEV_ASSERT_MSG(IsNearlyEqual(origIter->second->GetTranslation(), {0.f,0.f,0.f}),
                 "PoseOriginList.Rejigger.CurrentOriginShouldBeIdentity",
                 "Current origin (before rejigger) should be identity transform, not %s",
                 origIter->second->GetTranslation().ToString().c_str());

  origIter->second->GetTransform() = transform;
  origIter->second->SetParent(*newIter->second);
  origIter->second->SetName( origIter->second->GetName() + "_REJ");

  assert(origIter->second->IsRoot() == false);

  _current.originPtr = newIter->second.get();
  _current.originID  = newIter->first;
  DEV_ASSERT_MSG(_current.originID == newOriginID, "PoseOriginList.Rejigger.BadFinalCurrentOriginID",
                 "Expected:%d Got:%d", newOriginID, _current.originID);

  if(ANKI_DEV_CHEATS)
  {
    SanityCheckOwnership();
  }

  return RESULT_OK;
}

void PoseOriginList::Flatten(PoseOriginID_t worldOriginID)
{
  DEV_ASSERT_MSG(ContainsOriginID(worldOriginID), "PoseOriginList.Flatten.BadID", "ID:%d", worldOriginID);

  const Pose3d& worldOrigin = GetOriginByID(worldOriginID);

  for(auto & originAndIdPair : _origins)
  {
    Pose3d* origin = originAndIdPair.second.get();

    DEV_ASSERT(origin != nullptr, "PoseOriginList.Flatten.NullOrigin"); // Should REALLY never happen

    // if this origin has a parent and it's not the world origin, we want to update
    // this origin to be a direct child of the world origin
    if ( origin->HasParent() && !origin->IsChildOf(worldOrigin) )
    {
      // get WRT current origin, and store directly in place (we need to avoid creating a new Pose3d because we
      // don't want poses stored in the PoseOriginList to ever be deleted (which affects their ownership
      // of internal PoseTreeNodes)
      if ( origin->GetWithRespectTo(worldOrigin, *origin) )
      {
        const std::string& newName = origin->GetName() + "_FLT";
        origin->SetName( newName );
      }
    }
  }

  if(ANKI_DEV_CHEATS)
  {
    SanityCheckOwnership();
  }
}

bool PoseOriginList::SanityCheckOwnership() const
{
  bool allPosesOwned = true;
  for(auto const& originAndIdPair : _origins)
  {
    auto const& origin = originAndIdPair.second;
    allPosesOwned &= ANKI_VERIFY(origin->IsOwned(),
                                 "PoseOriginList.SanityCheckOwnership.UnownedOrigin",
                                 "Pose %d(%s) is unowned", origin->GetID(), origin->GetName().c_str());

    if(origin->HasParent())
    {
      const Pose3d& parent = origin->GetParent();
      allPosesOwned &= ANKI_VERIFY(parent.IsOwned(),
                                   "PoseOriginList.SanityCheckOwnership.OriginHasUnownedParent",
                                   "Pose %d(%s)'s parent %d(%s) is unowned",
                                   origin->GetID(), origin->GetName().c_str(),
                                   parent.GetID(), parent.GetName().c_str());
    }
  }

  return allPosesOwned;
}

} // namespace Anki
