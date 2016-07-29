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

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/poseOrigin.h"
#include "anki/common/basestation/math/poseOriginList.h"

namespace Anki
{
  
PoseOriginList::~PoseOriginList()
{
  for(auto & originAndIdPair : _origins)
  {
    Util::SafeDelete(originAndIdPair.second);
  }
}

bool PoseOriginList::ContainsOriginID(PoseOriginID_t ID) const
{
  auto iter = _origins.find(ID);
  return (iter != _origins.end());
}
  
PoseOriginID_t PoseOriginList::AddOrigin(PoseOrigin* origin)
{
  PoseOriginID_t ID = _nextID;
  
  AddOriginWithID(ID, origin);
  
  ++_nextID;
  
  return ID;
}

void PoseOriginList::AddOriginWithID(PoseOriginID_t ID, PoseOrigin* origin)
{
  _origins[ID] = origin;
  _idLUT[origin] = ID;
  
  _nextID = std::max(ID + 1, _nextID);
}
  
PoseOriginID_t PoseOriginList::GetOriginID(const PoseOrigin* origin) const
{
  auto iter = _idLUT.find(origin);
  if(iter == _idLUT.end())
  {
    return UnknownOriginID;
  }
  else
  {
    return iter->second;
  }
}

const PoseOrigin* PoseOriginList::GetOriginByID(PoseOriginID_t ID) const
{
  auto iter = _origins.find(ID);
  if(iter == _origins.end())
  {
    PRINT_NAMED_WARNING("PoseOriginList.GetOriginByID.BadOrigin",
                        "No origin stored for ID=%u", ID);
    return nullptr;
  }
  else
  {
    // NOTE: Recall that the ID is the index+1 (because 0 == unknown)
    return iter->second;
  }
}

void PoseOriginList::Flatten(const PoseOrigin* worldOrigin)
{
  for(auto & originAndIdPair : _origins)
  {
    Pose3d* origin = originAndIdPair.second;
    
    assert(origin != nullptr); // Should REALLY never happen
    
    // if this origin has a parent and it's not the world origin, we want to update
    // this origin to be a direct child of the world origin
    if ( (origin->GetParent() != nullptr) && (origin->GetParent() != worldOrigin) )
    {
      // get WRT current origin, and if we can (because our parent's origin is the current worldOrigin), then assign
      Pose3d iterWRTCurrentOrigin;
      if ( origin->GetWithRespectTo(*worldOrigin, iterWRTCurrentOrigin) )
      {
        const std::string& newName = origin->GetName() + "_FLT";
        *origin = iterWRTCurrentOrigin;
        origin->SetName( newName );
      }
    }
  }
}
  
} // namespace Anki
