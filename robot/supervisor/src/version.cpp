/** @file A minimal module just to send a version message output
 * @author Daniel Casner <daniel@anki.com>
 */

#include "version.h"
#include "anki/cozmo/robot/version.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

void Anki::Cozmo::Robot::SendVersionInfo()
{
  RobotInterface::RTIPVersionInfo msg;
  msg.version = COZMO_VERSION_COMMIT;
  msg.date    = COZMO_BUILD_DATE;
  msg.description_length = 0;
  while(BUILD_DATE[msg.description_length] != 0)
  {
    msg.description[msg.description_length] = BUILD_DATE[msg.description_length];
    msg.description_length++;
  }
  RobotInterface::SendMessage(msg);
}
