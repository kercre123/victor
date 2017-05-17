/**
 * File: behaviorChooserFactory
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Exactly what it sounds like, a factory for behavior choosers.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChooserFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChooserFactory_H__

#include "json/json-forwards.h"

#include <type_traits>

namespace Anki {
namespace Cozmo {

class Robot;
class IBehaviorChooser;

namespace BehaviorChooserFactory {

// creates and initializes the proper behavior chooser from the given config. Returns a new instance if
// the config is valid, or nullptr if the configuration was not valid. Freeing the memory is responsibility
// of the caller. This factory does not store the created pointers, however it provides a function for destruction
IBehaviorChooser* CreateBehaviorChooser(Robot& robot, const Json::Value& config);

// destroys the memory and sets the pointer to null. Allows passing a reference to a pointer
// of any class that derives from IBehaviorChooser. Returns void (as you can clearly see)
template <class BehaviorChooserClass>
typename std::enable_if<std::is_base_of<IBehaviorChooser, BehaviorChooserClass>::value, void>::type
DestroyBehaviorChooser(BehaviorChooserClass* &chooserPtr)
{
  delete chooserPtr;
  chooserPtr = nullptr;
}

}; // namespace BehaviorChooserFactory

} // namespace Cozmo
} // namespace Anki

#endif //
