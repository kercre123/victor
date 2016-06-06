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
#include "clad/types/activeObjectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace Anki {
namespace Cozmo {

class IExternalInterface;
template <typename Type>
class AnkiEvent;
class Robot;

namespace ExternalInterface {
  class MessageGameToEngine;
}
  
class BlockFilter
{
public: 
  BlockFilter(Robot* inRobot = nullptr);
  explicit BlockFilter(const std::string &path, Robot* inRobot = nullptr);

  void AddFactoryId(const FactoryID factoryId);

  void RemoveFactoryId(const FactoryID factoryId);
  void RemoveAllFactoryIds();

  bool ContainsFactoryId(FactoryID factoryId) const;

  bool Save(const std::string &path) const;
  bool Save() const;

  bool Load(const std::string &path);

  void SetEnabled(bool isEnabled) { _enabled = isEnabled; }
  bool IsEnabled() const { return _enabled; }
  
  void Init(const std::string &path, IExternalInterface* externalInterface);

private:
  
  bool ConnectToBlocks() const;
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  Robot*          _robot;
  FactoryIDArray  _blocks;
  std::string     _path;
  bool            _enabled;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  IExternalInterface* _externalInterface;

};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_Blocks_BlockFilter_H__) */
