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
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPuzzleMaze(const Json::Value& config);
  
  // ICozmoBehavior interface
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
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

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    // config speeds and vals
    int tileSize_px;
    int wallSize_px;
    int cozmoAvatarSize_px;
    float timeBetweenMazeSteps_Sec;
    float timePauseAtIntersectionMin_Sec;
    float timePauseAtIntersectionMax_Sec;
    float chanceWrongTurn;
    // Variables used in balance testing
    bool animateBetweenTiles;
  };

  struct DynamicVariables {
    DynamicVariables();
    AvatarState avatarState;
    // puzzle solving things
    std::vector< Point2i > path;
    MazeWalls currFacing;
    Point2i   currPos;
    Point2i   nextPos;
    float     nextStep_Sec;

    // Member variables
    MazeState state;

    // even if not animating between tiles it will treat it that way.
    float totalTimeInLastPuzzle_Sec;
    bool  isMazeSolved;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  // State management
  const char * EnumToString(const MazeState& state);
  
  // Update helpers
  Point2i DirFromEnum(MazeWalls dir);
  MazeWalls GetNextDir(const MazeWalls& currCell, const MazeWalls& currDir, Point2i& outMoveDir, bool rotateOrder = false);
  int GetNumberOfExitsFromCell(const MazeWalls& currCell);
  void SingleStepMaze();
  void UpdateMaze();
  void UpdateDisplay();
  
  // Draw helpers
  void DrawMaze(Vision::Image& image);
  void DrawCozmo(Vision::Image& image);
  
  // Action helpers
  void StartAnimation(const AnimationTrigger& animationTrigger);
  void StartAnimation(const AnimationTrigger& animationTrigger, const MazeState& nextState);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPuzzleMaze_H__
