/** Impelementation robot active object manager on Espressif
 * @author Daniel Casner <daniel@anki.com>
 */

#include "activeObjectManager.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

namespace Anki {
namespace Cozmo {
namespace ActiveObjectManager {

#define SLOT_UPDATE_SPACING_us (15*1000)

static u32 assignments[MAX_NUM_ACTIVE_OBJECTS];
static u32 needUpdate; ///< Bit flag, works as long as MAX_NUM_ACTIVE_OBJECTS < 32
static u32 lastBodyUpdateTime;

bool Init()
{
  needUpdate = 0;
  for (int i=0; i<MAX_NUM_ACTIVE_OBJECTS; ++i)
  {
    assignments[i] = 0;
    needUpdate |= 1 << i; 
  }
  lastBodyUpdateTime = system_get_time();
  return true;
}

void Update()
{
  if (needUpdate == 0) return;
  else if ((system_get_time() - lastBodyUpdateTime) < SLOT_UPDATE_SPACING_us) return;
  {
    for (u8 i=0; i<MAX_NUM_ACTIVE_OBJECTS; ++i)
    {
      if (needUpdate & (1 << i))
      {
        SetPropSlot msg;
        msg.factory_id = assignments[i];
        msg.slot = i;
        if (RobotInterface::SendMessage(msg)) needUpdate &= ~(1<<i);
        lastBodyUpdateTime = system_get_time();
        break;
      }
    }
  }
}

void SetSlots(const u8 start, const u8 count, const u32* const factory_ids)
{
  for (int i=0; i<count; ++i)
  {
    const int slot = i+start;
    if (slot >= MAX_NUM_ACTIVE_OBJECTS) break;
    if (assignments[slot] != factory_ids[i])
    {
      assignments[slot] = factory_ids[i];
      needUpdate |= 1<<slot;
    }
  }
}

void Disconnect(const u8 slot)
{
  const u32 none = 0;
  SetSlots(slot, 1, &none);
}

void DisconnectAll()
{
  u32 none[MAX_NUM_ACTIVE_OBJECTS] = {0};
  SetSlots(0, MAX_NUM_ACTIVE_OBJECTS, none);
}

} // ActiveObjectManager
} // Cozmo
} // Anki
