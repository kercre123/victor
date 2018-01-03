/**
 * File: behaviorPuzzleMaze.h
 *
 * Description: Cozmo solves mazes
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPuzzleMaze_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPuzzleMaze_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
// Forward declarations
namespace Anki {
  namespace Vision {
    class Image;
  }
}

#include <vector>


namespace Anki {
namespace Cozmo {
  
  /*
   Flag being set means there is wall there.
   */
enum MazeWalls : uint32_t
{
  North = 1 << 0, // 1
  West  = 1 << 1, // 2
  South = 1 << 2, // 4
  East  = 1 << 3, // 8
};
inline uint32_t operator&(MazeWalls lhs, MazeWalls rhs)
{
  return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
  
  
class BehaviorPuzzleMaze : public ICozmoBehavior
{
public:
  
  // usually only called from unit test/balancing tool.
  void SetAnimateBetweenPoints(bool isNormalMode);
  float GetTotalTimeFromLastRun();
  bool IsPuzzleCompleted();
  
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPuzzleMaze(const Json::Value& config);
  
  // ICozmoBehavior interface
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool ShouldCancelWhenInControl() const override { return false;}
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // Helper types
  enum class MazeState
  {
    Init,                   // Initialize everything
    GetIn,
    MazeStep,
    GetOut,
    Complete,
  };
  
  void TransitionToState(const MazeState& state);
  
private:
  
  // State of Cozmo's avatar as he's in the maze
  enum class AvatarState
  {
    MovingBetweenTiles,
    ThinkingAnim,
    WaitNextMove,
  };
  AvatarState _avatarState;
  
  // puzzle solving things
  std::vector< Point2i > _path;
  MazeWalls _currFacing;
  Point2i   _currPos;
  Point2i   _nextPos;
  float     _nextStep_Sec;
  
  // config speeds and vals
  int _tileSize_px;
  int _wallSize_px;
  int _cozmoAvatarSize_px;
  float _timeBetweenMazeSteps_Sec;
  float _timePauseAtIntersectionMin_Sec;
  float _timePauseAtIntersectionMax_Sec;
  float _chanceWrongTurn;
  
  // Variables used in balance testing
  bool _animateBetweenTiles;
  // even if not animating between tiles it will treat it that way.
  float _totalTimeInLastPuzzle_Sec;
  bool  _isMazeSolved;

  // Member variables
  MazeState _state = MazeState::Init;
  
  // State management
  const char * EnumToString(const MazeState& state);
  
  // Update helpers
  Point2i DirFromEnum(MazeWalls dir);
  MazeWalls GetNextDir(const MazeWalls& currCell, const MazeWalls& currDir, Point2i& outMoveDir, bool rotateOrder = false);
  int GetNumberOfExitsFromCell(const MazeWalls& currCell);
  void SingleStepMaze(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateMaze(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateDisplay(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Draw helpers
  void DrawMaze(Vision::Image& image, BehaviorExternalInterface& behaviorExternalInterface);
  void DrawCozmo(Vision::Image& image);
  
  // Action helpers
  void StartAnimation(BehaviorExternalInterface& behaviorExternalInterface, const AnimationTrigger& animationTrigger);
  void StartAnimation(BehaviorExternalInterface& behaviorExternalInterface, const AnimationTrigger& animationTrigger, const MazeState& nextState);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPuzzleMaze_H__
