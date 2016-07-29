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

#include "anki/common/types.h"
#include "anki/common/basestation/math/poseOrigin.h"

#include "util/helpers/templateHelpers.h"
#include "util/math/numericCast.h"

#include <vector>
#include <map>

namespace Anki {
  
class PoseOriginList
{
public:
  
  static const PoseOriginID_t UnknownOriginID = 0;
  
  PoseOriginList() { }
  ~PoseOriginList();
  
  // Takes over managing memory for given origin. ID is generated for you and returned.
  PoseOriginID_t AddOrigin(PoseOrigin* origin);
  
  // Allows you to add an origin with a specific ID, e.g. if you are mirroring another
  // origin lists via messages.
  void AddOriginWithID(PoseOriginID_t ID, PoseOrigin* origin);
  
  bool ContainsOriginID(PoseOriginID_t ID) const;
  
  const PoseOrigin* GetOriginByID(PoseOriginID_t ID) const;
  
  PoseOriginID_t GetOriginID(const PoseOrigin* origin) const;
  
  size_t GetSize() const { return _origins.size(); }
  
  void Flatten(const PoseOrigin* wrtOrigin);
  
private:
  
  PoseOriginID_t _nextID = UnknownOriginID + 1;
  
  std::map<PoseOriginID_t, PoseOrigin*> _origins;
  
  // Look up ID in by PoseOrigin*
  std::map<const PoseOrigin*, PoseOriginID_t> _idLUT;
  
}; // class PoseOriginList
  
  
} // namespace Anki

#endif // __Anki_Coretech_Common_PoseOriginList_H__
