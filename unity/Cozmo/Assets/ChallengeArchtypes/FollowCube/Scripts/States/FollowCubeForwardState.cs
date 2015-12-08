using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeForwardState : State {

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime;
    private FollowCubeGame _GameInstance;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FollowCubeGame;
    }

    public override void Update() {
      base.Update();
      if (Time.time - _LastSeenTargetTime > _GameInstance.NotSeenForgivenessThreshold) {
        _GameInstance.FailedAttempt();
      }

      if (_CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
        _LastSeenTargetTime = Time.time;

      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}