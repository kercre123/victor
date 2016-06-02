/**
 * File: searchForObjectAction
 *
 * Author: Mark Wesley
 * Created: 05/19/16
 *
 * Description: An action for Cozmo to look around to find either any cube or a specific cube
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Anki_Cozmo_Actions_SearchForObjectAction_H__
#define __Anki_Cozmo_Actions_SearchForObjectAction_H__


#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "clad/types/objectFamilies.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>


namespace Anki {
namespace Cozmo {

  
template <typename Type>
class AnkiEvent;
  

namespace ExternalInterface {
  class MessageEngineToGame;
}


class SearchForObjectAction : public IAction
{
public:
  
  SearchForObjectAction(Robot& robot, ObjectFamily desiredObjectFamily, ObjectID desiredObjectId, bool matchAnyObjectId);
  virtual ~SearchForObjectAction();
  
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return RobotActionType::SEARCH_FOR_OBJECT; }
  
  virtual u8 GetTracksToLock() const override { 
    return (u8)AnimTrackFlag::BODY_TRACK;
  }
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;

private:
  
  void HandleEvent(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
  
  CompoundActionSequential  _compoundAction;
  
  std::string   _name;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  ObjectFamily  _desiredObjectFamily;
  ObjectID      _desiredObjectId;
  bool          _matchAnyObjectId;
  bool          _foundObject;
  bool          _shouldPopIdle;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Actions_SearchForObjectAction_H__

