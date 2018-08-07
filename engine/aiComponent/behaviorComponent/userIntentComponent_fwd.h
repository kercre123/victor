/**
 * File: userIntentComponent_fwd.h
 *
 * Author: Brad Neuman
 * Created: 2018-05-16
 *
 * Description: Common and forward declarations for the user intent component
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_UserIntentComponent_Fwd_H__
#define __Engine_AiComponent_BehaviorComponent_UserIntentComponent_Fwd_H__

#include <stdint.h>
#include <memory>

namespace Anki {
namespace Vector {

class UserIntentComponent;

class UserIntent;
enum class UserIntentTag : uint8_t;

enum class UserIntentSource : uint8_t;

struct UserIntentData;

using UserIntentPtr = std::shared_ptr<const UserIntentData>;

}
}

#endif
