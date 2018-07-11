/**
 * File: behaviorPuzzleMaze.cpp
 *
 * Description: Puzzles! Mazes
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPuzzleMaze.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/puzzleComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/console/consoleInterface.h"

// Log options
#define LOG_CHANNEL    "Puzzle"

// Enable verbose trace messages?
#define ENABLE_TRACE 1

#if ENABLE_TRACE
#define LOG_TRACE LOG_INFO
#else
#define LOG_TRACE(...) {}
#endif

namespace Anki {
namespace Cozmo {
  
namespace {
  const char* kTileSizeKey = "tileSize_pixels";
  const char* kWallSizeKey = "wallSize_pixels";
  const char* kCozmoAvatarSizeKey = "cozmoAvatarSize_pixels";
  const char* kTimeBetweenTilesKey = "timeBetweenTiles_Sec";
  const char* kTimePauseAtIntersectionMinKey = "timePauseAtIntersectionMin_Sec";
  const char* kTimePauseAtIntersectionMaxKey = "timePauseAtIntersectionMax_Sec";
  const char* kChanceRandomWrongChoiceKey = "chanceRandomWrongChoice";
}
  
// Console parameters
#define CONSOLE_GROUP "Behavior.PuzzleMaze"
  
// Max time allowed in this behavior, in seconds
CONSOLE_VAR_RANGED(float, kPuzzleTimeout_sec, CONSOLE_GROUP, 24000.f, 0.f, 24000.f);
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Convert BehaviorPuzzleMaze to string
const char * BehaviorPuzzleMaze::EnumToString(const MazeState& state)
{
  switch (state) {
    case MazeState::Init:
      return "Init";
    case MazeState::GetIn:
      return "GetIn";
    case MazeState::MazeStep:
      return "MazeStep";
    case MazeState::GetOut:
      return "GetOut";
    case MazeState::Complete:
      return "Complete";
    default:
      DEV_ASSERT(false, "BehaviorPuzzleMaze.EnumToString.InvalidState");
      return "Invalid";
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPuzzleMaze::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  animateBetweenTiles = true;
  
  // Default values
  tileSize_px                    = 20;
  wallSize_px                    = 2;
  cozmoAvatarSize_px             = 6;
  timeBetweenMazeSteps_Sec       = 2.f;
  timePauseAtIntersectionMin_Sec = 0.5f;
  timePauseAtIntersectionMax_Sec = 1.0f;
  chanceWrongTurn                = 0.05f;

  // grab any values the JSON overrides
  JsonTools::GetValueOptional(config, kTileSizeKey,                tileSize_px);
  JsonTools::GetValueOptional(config, kWallSizeKey,                wallSize_px);
  JsonTools::GetValueOptional(config, kCozmoAvatarSizeKey,         cozmoAvatarSize_px);
  JsonTools::GetValueOptional(config, kTimeBetweenTilesKey,           timeBetweenMazeSteps_Sec);
  JsonTools::GetValueOptional(config, kTimePauseAtIntersectionMinKey, timePauseAtIntersectionMin_Sec);
  JsonTools::GetValueOptional(config, kTimePauseAtIntersectionMaxKey, timePauseAtIntersectionMax_Sec);
  JsonTools::GetValueOptional(config, kChanceRandomWrongChoiceKey,        chanceWrongTurn);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPuzzleMaze::DynamicVariables::DynamicVariables()
{
  avatarState = AvatarState::WaitNextMove;
  totalTimeInLastPuzzle_Sec = 0.f;
  isMazeSolved = false;
  nextStep_Sec = 0.f;
  state = MazeState::Init;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPuzzleMaze::BehaviorPuzzleMaze(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTileSizeKey,
    kWallSizeKey,
    kCozmoAvatarSizeKey,
    kTimeBetweenTilesKey,
    kTimePauseAtIntersectionMinKey,
    kTimePauseAtIntersectionMaxKey,
    kChanceRandomWrongChoiceKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transition to given state
void BehaviorPuzzleMaze::TransitionToState(const MazeState& state)
{
  if (_dVars.state != state) {
    LOG_TRACE("BehaviorPuzzleMaze.TransitionToState", "%s to %s", EnumToString(_dVars.state), EnumToString(state));
    SetDebugStateName(EnumToString(state));
    _dVars.state = state;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::StartAnimation(const AnimationTrigger& animationTrigger,
                                        const MazeState& nextState)
{
  LOG_TRACE("BehaviorPuzzleMaze.StartAnimation", "Start animation %s, nextState %s",
            AnimationTriggerToString(animationTrigger), EnumToString(nextState));
  
  // This should never be called when action is already in progress
  DEV_ASSERT(!IsControlDelegated(), "BehaviorPuzzleMaze.StartAnimation.ShouldBeInControl");
    
  auto callback = [this, nextState, animationTrigger] {
    LOG_TRACE("BehaviorPuzzleMaze.StartAnimation.Callback", "Finish animation %s, nextState %s",
              AnimationTriggerToString(animationTrigger), EnumToString(nextState));
    TransitionToState(nextState);
  };
  IActionRunner* action = new TriggerAnimationAction(animationTrigger);
  ANKI_VERIFY(DelegateIfInControl(action, callback), "BehaviorPuzzleMaze.StartAnimation.ShouldNotBeInControl","");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::StartAnimation(const AnimationTrigger& animationTrigger)
{
  StartAnimation(animationTrigger, _dVars.state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point2i BehaviorPuzzleMaze::DirFromEnum(MazeWalls dir)
{
  Point2i ret(0,0);
  if( dir == MazeWalls::North)
  {
    ret.y() = -1;
  }
  else if( dir == MazeWalls::West)
  {
    ret.x() = -1;
  }
  else if( dir == MazeWalls::South)
  {
    ret.y() = 1;
  }
  else if( dir == MazeWalls::East)
  {
    ret.x() = 1;
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int BehaviorPuzzleMaze::GetNumberOfExitsFromCell(const MazeWalls& currCell)
{
  int totalOpenings = 0;
  constexpr int kNumWalls = 4;
  for( int i = 0; i < kNumWalls; ++i )
  {
    int bitmask = 1 << i;
    if( (currCell & bitmask) == 0)
    {
      totalOpenings++;
    }
  }
  return totalOpenings;
}

  /* Right hand rule...
   if you can go right go right
   else if you can go forward go forward
   else if you can go left go left
   else if you can go back go back*/
  // func returns vector movement and direction...
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MazeWalls BehaviorPuzzleMaze::GetNextDir(const MazeWalls& currCell, const MazeWalls& currDir, Point2i& outMoveDir, bool rotateOrder)
{
  // priorities order
  std::vector<MazeWalls> orderCheck;
  if( currDir == MazeWalls::North )
  {
    orderCheck.insert(orderCheck.begin(), {MazeWalls::East, MazeWalls::North, MazeWalls::West, MazeWalls::South});
  }
  else if( currDir == MazeWalls::West )
  {
    orderCheck.insert(orderCheck.begin(), {MazeWalls::North, MazeWalls::West, MazeWalls::South, MazeWalls::East});
  }
  else if( currDir == MazeWalls::South )
  {
    orderCheck.insert(orderCheck.begin(), {MazeWalls::West, MazeWalls::South, MazeWalls::East, MazeWalls::North});
  }
  else if( currDir == MazeWalls::East )
  {
    orderCheck.insert(orderCheck.begin(), {MazeWalls::South, MazeWalls::East, MazeWalls::North, MazeWalls::West});
  }
  
  // Shuffle the order randomly, so the pattern isn't so obvious, basically go left first instead of right.
  if( rotateOrder )
  {
    LOG_TRACE("BehaviorPuzzleMaze.GetNextDir.RandomlyGoWrongWaySelected", "");
    std::swap(orderCheck[0], orderCheck[2]);
  }
  
  for( auto& wallCheck : orderCheck )
  {
    // if there is not wall go in that direction
    if( (currCell & wallCheck) == 0 )
    {
      outMoveDir = DirFromEnum(wallCheck);
      return wallCheck;
    }
  }
  PRINT_NAMED_WARNING("BehaviorPuzzleMaze.GetNextDir.BlockedCell", "No cell exit found, Puzzle is malformed.");
  return currDir;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Check if behavior can run at this time
bool BehaviorPuzzleMaze::WantsToBeActivatedBehavior() const
{
  return true;
}

//
// Behavior is starting. Reset behavior state.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::OnBehaviorActivated()
{
  LOG_TRACE("BehaviorPuzzleMaze.InitInternal", "Init behavior");
  
  const auto& currMaze = GetAIComp<PuzzleComponent>().GetCurrentMaze();
  _dVars.currFacing = MazeWalls::South;
  _dVars.currPos = currMaze._start;
  _dVars.path.clear();
  _dVars.path.emplace_back(_dVars.currPos);
  _dVars.nextStep_Sec = 0.f;
  _dVars.totalTimeInLastPuzzle_Sec = 0.f;
  _dVars.isMazeSolved = false;
  
  _dVars.nextPos = _dVars.currPos;
  
  TransitionToState(MazeState::GetIn);
  
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::SingleStepMaze()
{
  const auto& currMaze = GetAIComp<PuzzleComponent>().GetCurrentMaze();
  // This function is called once we've reached the end of an animation, so update our tile position
  _dVars.currPos = _dVars.nextPos;
  // step if we're not at the end.
  if( !(_dVars.currPos == currMaze._end) )
  {
    // we've reached the end of where we were moving.
    MazeWalls currcell = (MazeWalls)currMaze._maze[_dVars.currPos.y()][_dVars.currPos.x()];
    // This is more than just a hallway, we need to think about this.
    int numExits = GetNumberOfExitsFromCell(currcell) ;
    if( _dVars.avatarState == AvatarState::MovingBetweenTiles && numExits > 2)
    {
      _dVars.avatarState = AvatarState::ThinkingAnim;
      float thinkingTime = GetBEI().GetRNG().RandDblInRange(_iConfig.timePauseAtIntersectionMin_Sec, _iConfig.timePauseAtIntersectionMax_Sec);
      _dVars.nextStep_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + thinkingTime;
      // Even when doing our balancing and we skip animating we want this to simulate as if we had animated.
      _dVars.totalTimeInLastPuzzle_Sec += thinkingTime;
      
      LOG_TRACE("BehaviorPuzzleMaze.SingleStepMaze.ToThinking", "NumExits:%d x: %d y: %d, wait: %.2f", numExits, _dVars.currPos.x(), _dVars.currPos.y(), thinkingTime);
    }
    else
    {
      _dVars.avatarState = AvatarState::MovingBetweenTiles;
      _dVars.nextStep_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _iConfig.timeBetweenMazeSteps_Sec;
      // Even when doing our balancing and we skip animating we want this to simulate as if we had animated.
      _dVars.totalTimeInLastPuzzle_Sec += _iConfig.timeBetweenMazeSteps_Sec;
      
      bool makeWrongTurn = false;
      if( numExits > 2 )
      {
        makeWrongTurn = GetBEI().GetRNG().RandDbl() < _iConfig.chanceWrongTurn ;
      }
      Point2i moveDir;
      _dVars.currFacing = GetNextDir(currcell, _dVars.currFacing, moveDir, makeWrongTurn);
      _dVars.nextPos = _dVars.currPos + moveDir;
      _dVars.path.emplace_back(_dVars.currPos);
      LOG_TRACE("BehaviorPuzzleMaze.SingleStepMaze.ToMoving","at %d, %d, facing %d",_dVars.currPos.x(),_dVars.currPos.y(),(int)_dVars.currFacing);
    }
  }
  else
  {
    LOG_TRACE("BehaviorPuzzleMaze.SingleStepMaze.Solved", "");
    _dVars.isMazeSolved = true;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::UpdateMaze()
{
  // Do single step update, when done animating or when running in fast mode.
  if( (_dVars.nextStep_Sec < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() || !_iConfig.animateBetweenTiles) &&
     !_dVars.isMazeSolved)
  {
    SingleStepMaze();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::DrawMaze(Vision::Image& image)
{
  const auto& currMaze = GetAIComp<PuzzleComponent>().GetCurrentMaze();
  size_t h = currMaze.GetHeight();
  size_t w = currMaze.GetWidth();
  
  // if avatar is in state moving, this is correct. If in state waiting
  float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  float t = 1.f - ((_dVars.nextStep_Sec - currTime) / _iConfig.timeBetweenMazeSteps_Sec);
  
  float lerpedPosX = _dVars.currPos.x() + t * (_dVars.nextPos.x() - _dVars.currPos.x());
  float lerpedPosY = _dVars.currPos.y() + t * (_dVars.nextPos.y() - _dVars.currPos.y());
  // draw offset to mid screen.
  // convert from tile to pixel coordinates...
  int camX = (lerpedPosX * _iConfig.tileSize_px);
  int camY = (lerpedPosY * _iConfig.tileSize_px);
  
  int offsetX =  FACE_DISPLAY_WIDTH/2 - camX;
  int offsetY =  FACE_DISPLAY_HEIGHT/2 - camY;
  
  for( int y = 0; y < h; ++y )
  {
    for( int x = 0; x < w; ++x )
    {
      uint32_t wall = currMaze._maze[y][x];
      // set means a wall is there...
      if( (wall & MazeWalls::North) != 0 )
      {
        // draw top wall
        Rectangle<f32> rect( ( _iConfig.tileSize_px * x )+ offsetX,
                            ( _iConfig.tileSize_px * y ) + offsetY,
                            _iConfig.tileSize_px, _iConfig.wallSize_px);
        // Rect not line to control thinkness.
        image.DrawFilledRect(rect, NamedColors::WHITE);
      }
      if( (wall & MazeWalls::West) != 0 )
      {
        // Draw on the inner edges of the square
        Rectangle<f32> rect(( _iConfig.tileSize_px * x )+ offsetX,
                            ( _iConfig.tileSize_px * y ) + offsetY,
                            _iConfig.wallSize_px, _iConfig.tileSize_px);
        image.DrawFilledRect(rect, NamedColors::WHITE);
      }
      if( (wall & MazeWalls::South) != 0 )
      {
        Rectangle<f32> rect(( _iConfig.tileSize_px * x ) + offsetX,
                            ( _iConfig.tileSize_px * (y +1) ) -_iConfig.wallSize_px + offsetY,
                            _iConfig.tileSize_px, _iConfig.wallSize_px);
        image.DrawFilledRect(rect, NamedColors::WHITE);
      }
      if( (wall & MazeWalls::East) != 0 )
      {
        Rectangle<f32> rect(( _iConfig.tileSize_px * (x+1) ) - _iConfig.wallSize_px + offsetX,
                            ( _iConfig.tileSize_px * y ) + offsetY,
                            _iConfig.wallSize_px, _iConfig.tileSize_px);
        image.DrawFilledRect(rect, NamedColors::WHITE);
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::DrawCozmo(Vision::Image& image)
{
  // Cozmo is basically just always center screen dot. forcing world around him to scroll.
  // TODO: Loop an animation when in _dVars.avatarState = AvatarState::ThinkingAnim
  image.DrawFilledCircle(Point2f(FACE_DISPLAY_WIDTH/2+ _iConfig.tileSize_px/2 ,FACE_DISPLAY_HEIGHT/2+ _iConfig.tileSize_px/2 ),
                         NamedColors::WHITE, _iConfig.cozmoAvatarSize_px);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::UpdateDisplay()
{
  // Do we need to draw a new face?
  if (!IsControlDelegated() && _iConfig.animateBetweenTiles) {
    
    // Init background, height by width
    Vision::Image image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
    
    DrawMaze(image);
    DrawCozmo(image);
    
    GetBEI().GetAnimationComponent().DisplayFaceImage(image, AnimationComponent::DEFAULT_STREAMING_FACE_DURATION_MS, true);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  // Check elapsed time
  if (GetActivatedDuration() > kPuzzleTimeout_sec) {
    LOG_WARNING("BehaviorPuzzleMaze.UpdateIntern_Legacy", "Behavior has timed out");
    CancelSelf();
    return;
  }
  
  switch (_dVars.state) {
    case MazeState::Init:
    {
      // This should never happen. InitInternal (above) sets state to GetIn.
      DEV_ASSERT(_dVars.state != MazeState::Init, "BehaviorPuzzleMaze.Update.InvalidState");
      break;
    }
    case MazeState::GetIn:
    {
      if (!IsControlDelegated()) {
        StartAnimation(AnimationTrigger::CubePounceGetIn, MazeState::MazeStep);
      }
      break;
    }
    case MazeState::MazeStep:
    {
      // This is the actual game loop
      UpdateMaze();
      UpdateDisplay();
      break;
    }
    case MazeState::GetOut:
    {
      if (!IsControlDelegated()) {
        StartAnimation(AnimationTrigger::CubePounceGetOut, MazeState::Complete);
      }
      break;
    }
    case MazeState::Complete:
    {
      if (!IsControlDelegated()) {
        LOG_TRACE("BehaviorPuzzleMaze.Update.Complete", "Behavior complete");
        CancelSelf();
        return;
      }
    }
  }    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::OnBehaviorDeactivated()
{
  LOG_TRACE("BehaviorPuzzleMaze.StopInternal", "Stop behavior");
  
  // TODO: Record relevant DAS events here.
}

// usually only called from unit test/balancing data tool.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPuzzleMaze::SetAnimateBetweenPoints(bool isNormalMode)
{
  _iConfig.animateBetweenTiles = isNormalMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorPuzzleMaze::GetTotalTimeFromLastRun()
{
  return _dVars.totalTimeInLastPuzzle_Sec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPuzzleMaze::IsPuzzleCompleted()
{
  return _dVars.isMazeSolved;
}
  
} // namespace Cozmo
} // namespace Anki
