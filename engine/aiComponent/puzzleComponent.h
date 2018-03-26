/**
 * File: puzzleComponent.h
 *
 * Author: Molly Jameson
 * Created: 2017-11-15
 *
 * Description: AI component to handle cozmo's puzzles.  Mazes, etc.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_PuzzleComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_PuzzleComponent_H__

#include "coretech/common/engine/math/point.h"
#include "coretech/common/shared/types.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "json/json.h"

#include "util/helpers/noncopyable.h"

#include <string>
#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;

class PuzzleConfig : private Util::noncopyable {
protected:
  std::string _puzzleID; // just a filename
  std::string GetID() const { return _puzzleID; }
public:
  PuzzleConfig(std::string id);
};

// defines a maze
class MazeConfig : public PuzzleConfig  {
public:
  MazeConfig(std::string id);
  Result InitConfiguration(const Json::Value& config);
  size_t GetWidth() const;
  size_t GetHeight() const;
  std::vector< std::vector<uint32_t> > _maze;
  Point2i _start;
  Point2i _end;

};


class PuzzleComponent : public IDependencyManagedComponent<AIComponentID>, 
                        private Util::noncopyable
{
public:

  PuzzleComponent(Robot& robot);

  // Reads in several json configs in resources...
  Result InitConfigs();

  // returns the current maze the robot should do
  const MazeConfig& GetCurrentMaze() const;
  void CompleteCurrentMaze();
  size_t GetNumMazes() const;
  
private:

  // We're not sure how we're adding puzzles yet. So for now just keep a big list at start.
  using MazeList = std::vector< std::unique_ptr<MazeConfig> >;
  MazeList _mazes;

// which maze we are doing, must be set after the constructor is complete
  MazeList::const_iterator _currMaze;
  
  Robot& _robot;

};

}
}

#endif
