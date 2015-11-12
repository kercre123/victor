using UnityEngine;
using System.Collections;

namespace VisionTraining {

  public class RecognizeCubeState : State {

    private float _CubeInRectTime = 0.0f;
    private int _SelectedCubeId = -1;

    private Rect _VisionRect = new Rect(new Vector2(100.0f, 0.0f), new Vector2(50.0f, 50.0f));

    public override void Enter() {
      base.Enter();
      _SelectedCubeId = (_StateMachine.GetGame() as VisionTrainingGame).PickCube();
    }

    public override void Update() {
      base.Update();

      bool cubeInRectNow = false;

      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_CurrentRobot.VisibleObjects[i].ID == _SelectedCubeId) {
          if (_VisionRect.Contains((Vector2)_CurrentRobot.VisibleObjects[i].WorldPosition)) {
            cubeInRectNow = true;
          }
        }
      }

      if (cubeInRectNow) {
        _CubeInRectTime += Time.deltaTime;
      }
      else {
        _CubeInRectTime -= Time.deltaTime;
        if (_CubeInRectTime < 0.0f) {
          _CubeInRectTime = 0.0f;
        }
      }

      if (CubeInRect()) {
        _CurrentRobot.SendAnimation("majorWin", AnimationDone);
      }
    }

    public override void Exit() {
      base.Exit();
    }

    void AnimationDone(bool success) {
      _StateMachine.SetNextState(new RecognizeCubeState());
    }

    public bool CubeInRect() {
      return _CubeInRectTime > 3.0f;
    }
  }

}
