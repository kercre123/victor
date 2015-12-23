using UnityEngine;
using System.Collections;

namespace VisionTraining {

  public class RecognizeCubeState : State {

    private float _CubeInRectTime = 0.0f;
    private LightCube _CurrentTarget = null;

    private CubeVisionGame _GameInstance;
    private bool _CubeInZoneAnimationPlayed = false;

    // don't use the rect constructor position because
    // it is top left and not center of the rect.
    private Rect _VisionRect = new Rect();

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);

      _VisionRect.size = new Vector2(100.0f, 50.0f);
      _VisionRect.center = new Vector2(120.0f, 0.0f);

      _GameInstance = _StateMachine.GetGame() as CubeVisionGame;
      _GameInstance.ShowHowToPlaySlide("PlaceCubeVision");
    }

    public override void Update() {
      base.Update();

      bool cubeInRectNow = false;

      _GameInstance.Progress = _CubeInRectTime / 3.0f;

      _CurrentTarget = _GameInstance.CurrentTarget();

      if (_CurrentTarget != null) {
        Vector2 cubePositionCozmoSpace = (Vector2)_CurrentRobot.WorldToCozmo(_CurrentTarget.WorldPosition);
        if (_VisionRect.Contains(cubePositionCozmoSpace)) {
          cubeInRectNow = true;
        }
      }
      else {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleLoseAnimationDone));
        return;
      }

      if (cubeInRectNow) {
        _CubeInRectTime += Time.deltaTime;
        _CurrentTarget.SetLEDs(Color.white);
      }
      else {
        if (_CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
          _CurrentTarget.SetLEDs(Color.blue);
        }
        else {
          _CurrentTarget.SetLEDs(Color.black);
        }

        _CubeInRectTime -= Time.deltaTime;
        if (_CubeInRectTime < 0.0f) {
          _CubeInRectTime = 0.0f;
          _CubeInZoneAnimationPlayed = false;
        }
      }

      if (_CubeInRectTime > 0.3f && _CubeInZoneAnimationPlayed == false) {
        _CurrentRobot.SendAnimation(AnimationName.kEnjoyLight);
        _CubeInZoneAnimationPlayed = true;
      }

      if (_CubeInRectTime > 3.0f) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, HandleWinAnimationDone));
      }
    }

    private void HandleWinAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void HandleLoseAnimationDone(bool success) {
      _GameInstance.LostCube();
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new RecognizeCubeState(), 1, true, null);
      _StateMachine.SetNextState(initCubeState);
      _GameInstance.ShowHowToPlaySlide("ShowCubeVision");
    }

    public override void Exit() {
      base.Exit();
    }

  }

}
