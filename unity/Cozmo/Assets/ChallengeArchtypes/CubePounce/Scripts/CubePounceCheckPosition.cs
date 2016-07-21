using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public delegate void FixPositionCompleteHandler();

  public class CubePounceCheckPosition {
    private IRobot _CurrentRobot = null;
    private CubePounceGame _CubePounceGame = null;
    private FixPositionCompleteHandler _CheckAndFixCallback;

    public CubePounceCheckPosition(IRobot robot, CubePounceGame cubePounceGame, FixPositionCompleteHandler callback) {
      _CurrentRobot = robot;
      _CubePounceGame = cubePounceGame;
      _CheckAndFixCallback = callback;
    }

    public void ClearCallbacks() {
      _CurrentRobot.CancelCallback(TurnToCubeCallback);
      _CurrentRobot.CancelCallback(HandleDriveToCubeComplete);
      _CheckAndFixCallback = null;
    }

    public void CheckAndFixPosition() {
      bool posInRange = CozmoUtil.ObjectEdgeWithinXYTolerance(_CurrentRobot.WorldPosition, _CubePounceGame.GetCubeTarget(), _CubePounceGame.CubePlaceDist_mm, _CubePounceGame.PositionDiffTolerance_mm);
      if (!posInRange) {
        AlignToCube();
        return;
      }

      bool angleInRange = CozmoUtil.PointWithinXYAngleTolerance(
        _CurrentRobot.WorldPosition, _CubePounceGame.GetCubeTarget().WorldPosition, _CurrentRobot.Rotation.eulerAngles.z, _CubePounceGame.AngleTolerance_deg);
      if (!angleInRange) {
        TurnToCube();
        return;
      }

      if (null != _CheckAndFixCallback) {
        _CheckAndFixCallback();
      }
    }

    private void AlignToCube() {
      _CurrentRobot.AlignWithObject(_CubePounceGame.GetCubeTarget(), _CubePounceGame.CubePlaceDist_mm, HandleDriveToCubeComplete, useApproachAngle: false, usePreDockPose: true);
    }

    private void TurnToCube() {
      _CurrentRobot.TurnTowardsObject(_CubePounceGame.GetCubeTarget(), false, callback: TurnToCubeCallback);
    }

    private void TurnToCubeCallback(bool success) {
      if (null != _CheckAndFixCallback) {
        _CheckAndFixCallback();
      }
    }

    private void HandleDriveToCubeComplete(bool success) {
      if (success) {
        if (null != _CheckAndFixCallback) {
          _CheckAndFixCallback();
        }
      }
      else {
        AlignToCube();
      }
    }
  }
}
