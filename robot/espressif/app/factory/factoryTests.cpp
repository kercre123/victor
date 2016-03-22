/** Impelementation framework for factory test firmware on the Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
}
#include "anki/cozmo/robot/logging.h"
#include "factoryTests.h"
#include "face.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
namespace Cozmo {
namespace Factory {

static RobotInterface::FactoryTestMode mode;
static u32 lastExecTime;

bool Init()
{
  mode = RobotInterface::FTM_None;
  lastExecTime = system_get_time();
  return true;
}

void Update()
{
  const u32 now = system_get_time();
  switch (mode)
  {
    case RobotInterface::FTM_motorLifeTest:
    {
      if ((now - lastExecTime) > 1000000)
      {
        const u32 phase = now >> 20;
        const float wheelSpd = phase & 0x01 ? 200.0f : -200.0f;
        const float liftSpd  = phase & 0x02 ? 1.5f   : -1.5f;
        const float headSpd  = phase & 0x02 ? 2.5f   : -2.5f;
        RobotInterface::EngineToRobot msg;
        msg.tag = RobotInterface::EngineToRobot::Tag_drive;
        msg.drive.lwheel_speed_mmps = wheelSpd;
        msg.drive.rwheel_speed_mmps = wheelSpd;
        RTIP::SendMessage(msg);
        msg.tag = RobotInterface::EngineToRobot::Tag_moveLift;
        msg.moveLift.speed_rad_per_sec = liftSpd;
        RTIP::SendMessage(msg);
        msg.tag = RobotInterface::EngineToRobot::Tag_moveHead;
        msg.moveHead.speed_rad_per_sec = headSpd;
        RTIP::SendMessage(msg);
        lastExecTime = now;
      }
      break;
    }
    default:
    {
      break;
    }
  }
  return;
}

void Process_TestState(const RobotInterface::TestState& state)
{
  switch (mode)
  {
    // Do test stuff here
    default:
    {
      break;
    }
  }
}

void Process_EnterFactoryTestMode(const RobotInterface::EnterFactoryTestMode& msg)
{
  // Do cleanup on current mode
  switch (mode)
  {
    case RobotInterface::FTM_menus:
    {
      Face::FaceUnPrintf();
      break;
    }
    case RobotInterface::FTM_motorLifeTest:
    {
      RobotInterface::EngineToRobot msg;
      msg.tag = RobotInterface::EngineToRobot::Tag_stop;
      RTIP::SendMessage(msg);
      break;
    }
    default:
    {
      // Nothing to do
    }
  }
  switch (msg.mode)
  {
    // Do any entry here
    default:
    {
      // Nothing to do
    }
  }
  mode = msg.mode;
}

} // Factory
} // Cozmo
} // Anki
