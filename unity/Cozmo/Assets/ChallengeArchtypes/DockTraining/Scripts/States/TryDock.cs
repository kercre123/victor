using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class TryDock : State {

    private LightCube _DockTarget;
    private float _LastSeenTargetTime;
    private DockTrainingGame _GameInstance;

    public void Init(LightCube dockTarget) {
      _DockTarget = dockTarget;
    }

    public override void Enter() {
      base.Enter();
      _CurrentRobot.DriveWheels(15.0f, 15.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _LastSeenTargetTime = Time.time;
      _GameInstance = _StateMachine.GetGame() as DockTrainingGame;
    }

    public override void Update() {
      base.Update();
      float distance = Vector3.Distance(_CurrentRobot.WorldPosition, _DockTarget.WorldPosition);
      float relDot = Vector2.Dot((Vector2)(_CurrentRobot.Forward).normalized, (Vector2)((_DockTarget.WorldPosition - _CurrentRobot.WorldPosition).normalized));

      bool targetVisible = _CurrentRobot.VisibleObjects.Contains(_DockTarget);

      if (targetVisible) {
        _LastSeenTargetTime = Time.time;
        if (distance < 53.0f && relDot > 0.88f) {
          _StateMachine.SetNextState(new AnimationState(AnimationName.kEnjoyLight, HandleWinAnimationDone));
        }
      }
      else {
        if (Time.time - _LastSeenTargetTime > 1.5f) {
          _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleLoseAnimationDone));
        }
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void HandleLoseAnimationDone(bool success) {
      _CurrentRobot.GotoPose(_GameInstance.StartingPosition(), _GameInstance.StartingRotation(), HandleBackToStartPos);
    }

    private void HandleBackToStartPos(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
      (_StateMachine.GetGame() as DockTrainingGame).DockFailed();
    }

    private void HandleWinAnimationDone(bool success) {
      (_StateMachine.GetGame() as DockTrainingGame).RaiseMiniGameWin();
    }
  }
}