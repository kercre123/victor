using UnityEngine;
using System.Collections;

namespace DockTraining {

  // Determines if it should move left/right on wave or try to dock if it is close enough.
  public class DetermineNextAction : State {

    private float _WaveTimeAccumulator = 0.0f;
    private float _LastWaveTime = 0.0f;

    private DockTrainingGame _DockTrainingGame;

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
      RobotEngineManager.Instance.OnObservedMotion += HandleObservedMotion;

      _DockTrainingGame = _StateMachine.GetGame() as DockTrainingGame;

      _DockTrainingGame.ShowHowToPlaySlide("WaveToLineUp");

      LightCube currentTarget = _DockTrainingGame.GetCurrentTarget();
      if (currentTarget != null) {
        currentTarget.SetLEDs(Color.white);
      }
    }

    public override void Update() {
      base.Update();
      if (_DockTrainingGame.ShouldTryDock()) {
        if (_DockTrainingGame.ShouldTryDockSucceed()) {
          // should try to actually successfully dock.
          TryDock tryDockState = new TryDock();
          tryDockState.Init(_DockTrainingGame.GetCurrentTarget());
          _StateMachine.SetNextState(tryDockState);
        }
        else {
          // we are too far off, just fail spectacularly
          _StateMachine.SetNextState(new FailSpectacularDock());
        }
      }
      if (_DockTrainingGame.GetCurrentTarget() == null) {
        _StateMachine.SetNextState(new FailSpectacularDock());
      }
    }

    public override void Exit() {
      base.Exit();
      RobotEngineManager.Instance.OnObservedMotion -= HandleObservedMotion;
    }

    private void HandleObservedMotion(Vector2 pos) {
      // wave accumulator is high enough for us to go left or right.
      if (_WaveTimeAccumulator > 0.5f) {
        SteerState steerState = new SteerState();
        steerState.Init(pos.x, 0.8f, new DetermineNextAction());
        _StateMachine.SetNextState(steerState);
      }

      float dt = Mathf.Min(0.3f, Time.time - _LastWaveTime);

      _WaveTimeAccumulator += dt;
      _LastWaveTime = Time.time;
    }
  }

}
