/**
 * File: needsConfig
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: Configuration data for the Cozmo Needs system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__
#define __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__

#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {


namespace ExternalInterface {
  class MessageGameToEngine;
}
  
  
class Robot;

  
class NeedsConfig
{
public:
  NeedsConfig() { foo = 2; }
private:
  int foo = 3;
//  using BracketThresholds = std::vector<float>;
//  using CurNeedsBrackets = std::map<NeedId, BracketThresholds>;
//  CurNeedsBrackets _curNeedsBracketsCache;
  // todo: Fill this out as I implement features; tunable constants, etc.
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__

