/**
* File: delegationComponent.h
*
* Author: Kevin M. Karol
* Created: 09/22/17
*
* Description: Component which handles delegation requests from behaviors
* and forwards them to the appropriate behavior/robot system
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__


namespace Anki {
namespace Cozmo {

// Forward Declaration
  
class DelegationComponent {
public:
  DelegationComponent();
  
  virtual ~DelegationComponent(){};
private:

  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
