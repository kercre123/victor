using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeForwardState : State {

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime;
    private FollowCubeGame _GameInstance;
    private Vector3 _RobotStartPosition;
    private float _WinDistanceThreshold = 150.0f;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FollowCubeGame;
      _RobotStartPosition = _CurrentRobot.WorldPosition;
    }

    public override void Update() {
      base.Update();
      if (Time.time - _LastSeenTargetTime > _GameInstance.NotSeenForgivenessThreshold) {
        _GameInstance.FailedAttempt();
        return;
      }


      if (Vector3.Distance(_CurrentRobot.WorldPosition, _RobotStartPosition) > _WinDistanceThreshold) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kEnjoyPattern, HandleTaskCompleteAnimation);
        _StateMachine.SetNextState(animState);
      }

      if (_CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
        _LastSeenTargetTime = Time.time;

      }

    }

    private void HandleTaskCompleteAnimation(bool success) {
      _StateMachine.SetNextState(new FollowCubeBackwardState());
      _GameInstance.CurrentFollowTask = FollowCubeGame.FollowTask.Backwards;
    }

    public override void Exit() {
      base.Exit();
    }
  }
}