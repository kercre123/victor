/**
 * File: pathDolerOuter.h
 *
 * Author: Brad Neuman
 * Created: 2014-06-09
 *
 * Description: This object keeps track of the full-length path on the
 * basestation, and send it out bit by bit to the robot in chunks that
 * it can handle
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __COZMO_PATH_DOLER_OUTER_H__
#define __COZMO_PATH_DOLER_OUTER_H__

#include "anki/planning/shared/path.h"

namespace Anki {
namespace Cozmo {

class IMessageHandler;

class PathDolerOuter
{
public:

  PathDolerOuter(IMessageHandler* msgHandler, RobotID_t robotID_);

  // Updates the current path and will begins doling it out
  // immediately. NOTE: robot should already have a clear path before
  // this is called
  void SetPath(const Planning::Path& path);

  // Doles out the path bit by bit to the robot. The argument is the
  // segment the robot is on (relative to the path it has)
  void Update(s8 indexOnRobotPath);

protected:

  void Dole(size_t validRobotPathLength, size_t numToDole);

  Planning::Path path_;

  // the robot's index 0 corresponds to this index in our path
  size_t robotIdx_;

  size_t pathSizeOnRobot_;
  size_t pathSizeOnBasestation_;

  // A reference to the MessageHandler that the robot uses for outgoing comms
  IMessageHandler* msgHandler_;
  RobotID_t robotID_;

};

}
}

#endif
