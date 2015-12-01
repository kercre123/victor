using UnityEngine;
using System.Collections;

namespace DockTraining {

  // Determines if it should move left/right on wave or
  // try to dock if it is close enough.
  public class DetermineNextAction : State {

    private float _WaveTimeAccumulator = 0.0f;
    private float _LastWaveTime = 0.0f;
    private float _LeftAccumulator = 0.0f;
    private float _RightAccumulator = 0.0f;

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
      RobotEngineManager.Instance.OnObservedMotion += HandleObservedMotion;
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }

    private void HandleObservedMotion(float x, float y) {
      // wave accumulator is high enough for us to 
      // go left or right.
      if (_WaveTimeAccumulator > 0.5f) {
        SteerState steerState = new SteerState();
        if (_LeftAccumulator > _RightAccumulator) {
          steerState.Init(25.0f, 20.0f, 1.0f, new DetermineNextAction());
        }
        else {
          steerState.Init(20.0f, 25.0f, 1.0f, new DetermineNextAction());
        }
        _StateMachine.SetNextState(steerState);
      }

      float dt = Mathf.Min(0.3f, Time.time - _LastWaveTime);

      if (x > 0.0f) {
        _LeftAccumulator += dt;
      }
      else {
        _RightAccumulator += dt;
      }

      _WaveTimeAccumulator += dt;
      _LastWaveTime = Time.time;
    }
  }

}