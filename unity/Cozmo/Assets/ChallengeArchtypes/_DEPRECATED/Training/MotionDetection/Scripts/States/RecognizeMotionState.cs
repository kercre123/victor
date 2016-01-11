using UnityEngine;
using System.Collections;

namespace MotionDetection {

  public class RecognizeMotionState : State {

    private float _TimeAllowedBetweenWaves;
    private float _TotalWaveTime;

    private float _MotionInBoxTime = 0.0f;

    private float _LastMotionDetectedTime = 0.0f;

    private Vector2 _LeftEyeOuterPosition;
    private Vector2 _RightEyeOuterPosition;

    private Vector2 _LeftEyeInnerPosition;
    private Vector2 _RightEyeInnerPosition;


    private ProceduralEyeParameters _LeftEye = ProceduralEyeParameters.MakeDefaultLeftEye();
    private ProceduralEyeParameters _RightEye = ProceduralEyeParameters.MakeDefaultRightEye();

    // this will be the top middle of the screen
    // -1    -.5     0      .5     1
    // +------+------+------+------+ 1
    // |      |......|......|      |
    // |      |...... ......|      |
    // |      |......|......|      |
    // |      |...... ......|      |
    // |      |......|......|      |
    // +- - - + - - -+- - - + - - -+ 0
    // |      |......|......|      |
    // |      |...... ......|      |
    // |      +------+------+      | -.5
    // |                           |
    // |             |             |
    // +-------------+-------------+ -1
    //
    private Rect _VisionBounds = new Rect(-0.5f, -0.5f, 1f, 1.5f);

    public RecognizeMotionState(float timeAllowedBetweenWaves, float totalWaveTime) {
      _TimeAllowedBetweenWaves = timeAllowedBetweenWaves;
      _TotalWaveTime = totalWaveTime;

      _LeftEyeInnerPosition = _LeftEye.EyeCenter;
      _LeftEyeOuterPosition = _LeftEye.EyeCenter - Vector2.one * 20f;

      _RightEyeInnerPosition = _RightEye.EyeCenter;
      _RightEyeOuterPosition = _RightEye.EyeCenter + Vector2.one * 20f;

    }

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(0.3f);

      RobotEngineManager.Instance.OnObservedMotion += OnMotionDetected;
    }

    public override void Update() {
      base.Update();

      bool motionInBox = Time.time - _LastMotionDetectedTime < _TimeAllowedBetweenWaves;

      if (!motionInBox) {
        _MotionInBoxTime -= Time.deltaTime;
        if (_MotionInBoxTime < 0.0f) {
          _MotionInBoxTime = 0.0f;
        }
      }

      float lerpVal = Mathf.Clamp01(_MotionInBoxTime / _TotalWaveTime);

      _StateMachine.GetGame().Progress = lerpVal;

      _LeftEye.EyeCenter = Vector2.Lerp(_LeftEyeOuterPosition, _LeftEyeInnerPosition, lerpVal);
      _RightEye.EyeCenter = Vector2.Lerp(_RightEyeOuterPosition, _RightEyeInnerPosition, lerpVal);

      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, _LeftEye, _RightEye);

      if (MotionInBoxForRequiredTime()) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, HandleWinAnimationDoneHandler));
      }
    }

    private void HandleWinAnimationDoneHandler(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void OnMotionDetected(Vector2 pos) {
      if (_VisionBounds.Contains(pos)) {
        _LastMotionDetectedTime = Time.time;
        _MotionInBoxTime += 0.25f;
      }
    }


    public override void Exit() {

      RobotEngineManager.Instance.OnObservedMotion -= OnMotionDetected;
      base.Exit();
    }

    public bool MotionInBoxForRequiredTime() {
      return _MotionInBoxTime > _TotalWaveTime;
    }
  }

}
