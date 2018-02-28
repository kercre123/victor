/**
 * File: puzzleComponent.cpp
 *
 * Author: Molly Jameson
 * Created: 2017-11-15
 *
 * Description: AI component to handle cozmo's puzzles. Mazes, etc.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/
#include "engine/aiComponent/puzzleComponent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/robot.h"
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PuzzleConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
bool GetVectorofVectorsOptional(const Json::Value& config, const std::string& key, std::vector<std::vector<T>>& values)
{
  const Json::Value& child(config[key]);
  if(child.isNull() || !child.isArray())
    return false;
  values.reserve(child.size());
  for(auto const& element : child) {
    if(element.isNull() || !element.isArray())
      return false;
    std::vector<T> innerVec;
    innerVec.reserve(element.size());
    for(auto const& innerelement : element) {
      innerVec.emplace_back(JsonTools::GetValue<T>(innerelement));
    }
    values.emplace_back(innerVec);
  }
  
  return true;
}

PuzzleConfig::PuzzleConfig(std::string id) : _puzzleID(id)
{
}

MazeConfig::MazeConfig(std::string id) : PuzzleConfig(id)
{
}

Result MazeConfig::InitConfiguration(const Json::Value& config)
{
  GetVectorofVectorsOptional(config,"grid",_maze);
  // make sure the grid is rectangular
  size_t mazeWidth = GetWidth();
  DEV_ASSERT(std::all_of(_maze.begin(), _maze.end(), [mazeWidth]( const auto& vec ) { return vec.size() == mazeWidth; } ),
             "MazeConfig.InitConfiguration.NonRectangleMaze");
  JsonTools::GetValueOptional(config, "entryx", _start.x());
  JsonTools::GetValueOptional(config, "entryy", _start.y());
  JsonTools::GetValueOptional(config, "exitx", _end.x());
  JsonTools::GetValueOptional(config, "exity", _end.y());
  return RESULT_OK;
}
  
size_t MazeConfig::GetWidth() const
{
  if( _maze.size() > 0 )
  {
    return _maze[0].size();
  }
  return 0;
}
size_t MazeConfig::GetHeight() const
{
  return _maze.size();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PuzzleComponent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PuzzleComponent::PuzzleComponent(Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::Puzzle)
, _robot(robot)
{
  _currMaze = _mazes.end();
}

Result PuzzleComponent::InitConfigs()
{
  if( _robot.GetContextDataPlatform() == nullptr )
  {
    PRINT_CH_INFO("Behaviors", "PuzzleComponent.InitConfigs.NoDataPlatform","");
    return RESULT_FAIL;
  }
  
  const std::string puzzlePath = "config/engine/puzzles";
  static const std::vector<const char*> fileExts = {"json"};
  const std::string mazeFolder = _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Resources, 
                                        puzzlePath + "/maze");
  // TODO: refactor loading to robotDataLoader once we figure out how we're actually getting puzzles.
  // Pre-existings vs stored on cards. And if we're storing based on state.
  auto filePaths = Util::FileUtils::FilesInDirectory(mazeFolder, true, fileExts, true);
  for (const auto& path : filePaths) {
    Json::Value jsonRoot;
    const bool success = _robot.GetContextDataPlatform()->readAsJson(path, jsonRoot);
    if( success )
    {
      std::unique_ptr<MazeConfig> mazeConfig = std::make_unique<MazeConfig>(path);
      Result res = mazeConfig->InitConfiguration( jsonRoot );
      if( res != RESULT_OK ) {
        return res;
      }
      _mazes.emplace_back( std::move( mazeConfig ) );
    }
  }

  PRINT_CH_INFO("Behaviors", "PuzzleComponent.InitConfigs",
                "Loaded %zu configs", _mazes.size());
  
  if( _mazes.empty() ) {
    return RESULT_FAIL;
  }
  else {
    _currMaze = _mazes.begin();
    return RESULT_OK;
  }
}

const MazeConfig& PuzzleComponent::GetCurrentMaze() const
{
  // there should always be a puzzle
  DEV_ASSERT(_currMaze != _mazes.end(), "PuzzleComponent.GetCurrentMaze.NoMaze");
  return (*(*_currMaze).get());
}

void PuzzleComponent::CompleteCurrentMaze()
{
  if( _currMaze != _mazes.end() ) {
    if( _currMaze != --(_mazes.end()) ) {
      ++_currMaze;
    }
  }
}
  
size_t PuzzleComponent::GetNumMazes() const
{
  return _mazes.size();
}

}
}
