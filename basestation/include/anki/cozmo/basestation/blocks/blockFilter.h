/**
 * File: blockFilter.h
 *
 * Author: Molly Jameson
 * Date:   2/18/2016
 *
 * Description: blockFilter is a serializable set of objectIDs.
 *                So Cozmo only uses certain blocks.
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Cozmo_Basestation_Blocks_BlockFilter_H__
#define __Cozmo_Basestation_Blocks_BlockFilter_H__

#include "anki/common/basestation/objectIDs.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace Anki {
namespace Cozmo {

class BlockFilter {
public: 
  using ObjectIDSet = std::unordered_set<ObjectID>;

  BlockFilter();
  explicit BlockFilter(const ObjectIDSet &objectIds);
  explicit BlockFilter(const std::string &path);

  void AddObjectId(const ObjectID objectId);
  void AddObjectIds(const ObjectIDSet &objectIds);

  void RemoveObjectId(const ObjectID objectId);
  void RemoveObjectIds(const ObjectIDSet &objectIds);
  void RemoveAllObjectIds();

  void GetObjectIds(ObjectIDSet &objectIds) const;
  bool ContainsObjectId(ObjectID objectId) const;
  bool IsEmpty() const { return _blocks.empty(); }

  bool Save(const std::string &path) const;
  bool Save() const;

  bool Load(const std::string &path);

  void SetEnabled(bool isEnabled) { _enabled = isEnabled; }
  bool IsEnabled() const { return _enabled; }
  
  void SetPath(const std::string &path) { _path = path; }

private:
  ObjectIDSet _blocks;
  std::string _path;
  bool _enabled;

};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_Blocks_BlockFilter_H__) */
