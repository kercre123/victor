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
namespace RobotInterface {
class MessageHandler;
}
class PathDolerOuter
{
public:

  PathDolerOuter(RobotInterface::MessageHandler* msgHandler, RobotID_t robotID_);

  // Updates the current path and will begins doling it out
  // immediately. NOTE: robot should already have a clear path before
  // this is called
  void SetPath(const Planning::Path& path);

  // change the path without resetting the dole index. Should only be used if we are only
  // updating segments on the current path that have not been doled
  void ReplacePath(const Planning::Path& newPath);

  void ClearPath();

  // Doles out the path bit by bit to the robot. The argument is the
  // currPathIdx: The current (absolute) segment index that the robot is traversing.
  void Update(const s8 currPathIdx);

  const Planning::Path& GetPath() const {return path_;}
  s16 GetLastDoledIdx() const {return lastDoledSegmentIdx_;}

protected:

  void Dole(size_t numToDole);

  Planning::Path path_;

  size_t pathSizeOnBasestation_;

  s16 lastDoledSegmentIdx_;
  
  // A reference to the MessageHandler that the robot uses for outgoing comms
  RobotInterface::MessageHandler* msgHandler_;
  RobotID_t robotID_;

};

}
}

#endif
