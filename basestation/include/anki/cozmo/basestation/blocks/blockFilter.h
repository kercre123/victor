/**
 * File: blockFilter.h
 *
 * Author: Molly Jameson
 * Date:   2/18/2016
 *
 * Description: blockFilter is a serializable set of factoryIds.
 *                So Cozmo only uses certain blocks.
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Cozmo_Basestation_Blocks_BlockFilter_H__
#define __Cozmo_Basestation_Blocks_BlockFilter_H__

#include "anki/cozmo/basestation/activeCube.h"
#include "util/signals/simpleSignal_fwd.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace Anki {
namespace Cozmo {

class IExternalInterface;
template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}
  
class BlockFilter
{
public: 
  using FactoryIDSet = std::unordered_set<FactoryID>;

  BlockFilter();
  explicit BlockFilter(const FactoryIDSet &factoryIds);
  explicit BlockFilter(const std::string &path);

  void AddFactoryId(const FactoryID factoryId);
  void AddFactoryIds(const FactoryIDSet &factoryIds);

  void RemoveFactoryId(const FactoryID factoryId);
  void RemoveFactoryIds(const FactoryIDSet &factoryIds);
  void RemoveAllFactoryIds();

  void GetFactoryIds(FactoryIDSet &factoryIds) const;
  bool ContainsFactoryId(FactoryID factoryId) const;
  bool IsEmpty() const { return _blocks.empty(); }

  bool Save(const std::string &path) const;
  bool Save() const;

  bool Load(const std::string &path);

  void SetEnabled(bool isEnabled) { _enabled = isEnabled; }
  bool IsEnabled() const { return _enabled; }
  
  void Init(const std::string &path, IExternalInterface* externalInterface);

private:
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  FactoryIDSet _blocks;
  std::string _path;
  bool        _enabled;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  IExternalInterface* _externalInterface;

};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_Blocks_BlockFilter_H__) */
