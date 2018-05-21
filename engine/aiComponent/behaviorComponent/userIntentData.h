/**
 * File: userIntentData.h
 *
 * Author: Brad Neuman
 * Created: 2018-05-16
 *
 * Description: Definition of intent data. This file exists to hide the dependency on the clad-generated
 *              intent file from most users
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_UserIntentData_H__
#define __Engine_AiComponent_BehaviorComponent_UserIntentData_H__

#include "clad/types/behaviorComponent/userIntent.h"

namespace Anki {
namespace Cozmo {

struct UserIntentData
{
  UserIntentData(UserIntent&& intent, const UserIntentSource& source) : intent(intent), source(source) {}
  UserIntentData(const UserIntent& intent, const UserIntentSource& source) : intent(intent), source(source) {}
  
  UserIntent intent;
  UserIntentSource source;

  // when this intent is activated, this will be set to a unique (incrementing) ID.
  size_t activationID = 0;
};


}
}

#endif
