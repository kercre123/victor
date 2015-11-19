using UnityEngine;
using System.Collections;

namespace VisionTraining {

  public class RecognizeCubeState : State {

    private float _CubeInRectTime = 0.0f;
    private int _SelectedCubeId = -1;

    // don't use the rect constructor position because
    // it is top left and not center of the rect.
    private Rect _VisionRect = new Rect();

    public override void Enter() {
      base.Enter();
      _SelectedCubeId = (_StateMachine.GetGame() as VisionTrainingGame).PickCube();
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);

      _VisionRect.size = new Vector2(100.0f, 50.0f);
      _VisionRect.center = new Vector2(120.0f, 0.0f);
      Debug.Log(_VisionRect.position);


    }

    public override void Update() {
      base.Update();

      bool cubeInRectNow = false;
      bool cubeInViewNow = false;

      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_CurrentRobot.VisibleObjects[i].ID == _SelectedCubeId) {
          Vector2 cubePositionCozmoSpace = (Vector2)_CurrentRobot.WorldToCozmo(_CurrentRobot.VisibleObjects[i].WorldPosition);
          Debug.Log(cubePositionCozmoSpace);
          if (_VisionRect.Contains(cubePositionCozmoSpace)) {
            cubeInRectNow = true;
          }
          cubeInViewNow = true;
        }
      }

      if (cubeInRectNow) {
        _CubeInRectTime += Time.deltaTime;
        _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(Color.white));
      }
      else {
        if (cubeInViewNow) {
          _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(Color.blue));
        }
        else {
          _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(Color.red));
        }

        _CubeInRectTime -= Time.deltaTime;
        if (_CubeInRectTime < 0.0f) {
          _CubeInRectTime = 0.0f;
        }
      }

      if (CubeInRect()) {
        _StateMachine.SetNextState(new CelebrateState());
      }
    }

    public void SetDistanceBackpackLights() {
      LightCube pickedCubeInView = null;
      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_CurrentRobot.VisibleObjects[i].ID == _SelectedCubeId) {
          pickedCubeInView = _CurrentRobot.VisibleObjects[i] as LightCube;
        }
      }

      if (pickedCubeInView != null) {
        // set backpack light based on distance

      }
      else {
        // set backpack light off
      }
    }

    public override void Exit() {
      base.Exit();
    }

    public bool CubeInRect() {
      return _CubeInRectTime > 3.0f;
    }
  }

}
