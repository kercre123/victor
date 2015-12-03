using UnityEngine;
using System.Collections;

namespace MotionDetection {

  public class RecognizeMotionState : State {

    private float _TimeAllowedBetweenWaves;
    private float _TotalWaveTime;

    private float _MotionInBoxTime = 0.0f;

    private float _LastMotionDetectedTime = 0.0f;

    // don't use the rect constructor position because
    // it is top left and not center of the rect.
    private Rect _VisionBounds = new Rect();

    public RecognizeMotionState(float timeAllowedBetweenWaves, float totalWaveTime) {
      _TimeAllowedBetweenWaves = timeAllowedBetweenWaves;
      _TotalWaveTime = totalWaveTime;
    }
      
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(0.0f);

      // this will be the middle quarter of the screen
      _VisionBounds.size = Vector2.one;
      _VisionBounds.center = Vector2.zero;

      RobotEngineManager.Instance.OnObservedMotion += OnMotionDetected;
    }
      
    public override void Update() {
      base.Update();

      bool motionInBox = Time.time - _LastMotionDetectedTime < _TimeAllowedBetweenWaves;


      if (motionInBox) {
        _MotionInBoxTime += Time.deltaTime;
      }
      else {
        // move eyes apart?

        _MotionInBoxTime -= Time.deltaTime;
        if (_MotionInBoxTime < 0.0f) {
          _MotionInBoxTime = 0.0f;
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
