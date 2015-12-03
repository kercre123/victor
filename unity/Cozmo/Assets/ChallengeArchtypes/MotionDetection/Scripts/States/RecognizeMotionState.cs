using UnityEngine;
using System.Collections;

namespace MotionDetection {

  public class RecognizeMotionState : State {

    private float _TimeAllowedBetweenWaves;
    private float _TotalWaveTime;

    private float _MotionInBoxTime = 0.0f;

    private float _LastMotionDetectedTime = 0.0f;

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


      if (motionInBox) {
        if (Mathf.Approximately(_MotionInBoxTime, 0)) {
          _CurrentRobot.SendAnimation(AnimationName.kEnjoyPattern);
        }

        _MotionInBoxTime += Time.deltaTime;
      }
      else {
        // move eyes apart?
        bool wasWaving = _MotionInBoxTime > 0;

        _MotionInBoxTime -= Time.deltaTime;
        if (_MotionInBoxTime < 0.0f) {
          _MotionInBoxTime = 0.0f;

          if (wasWaving) {
            _CurrentRobot.SendAnimation(AnimationName.kShocked);
          }
        }
      }

      if (MotionInBoxForRequiredTime()) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDoneHandler);
        _StateMachine.SetNextState(animState);
      }
    }

    private void HandleWinAnimationDoneHandler(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void OnMotionDetected(Vector2 pos) {
      if (_VisionBounds.Contains(pos)) {
        _LastMotionDetectedTime = Time.time;
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
