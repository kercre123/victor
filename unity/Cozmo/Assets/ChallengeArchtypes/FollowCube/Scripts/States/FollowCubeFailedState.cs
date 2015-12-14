using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeFailedState : State {
    private FollowCubeGame _GameInstance;

    public override void Enter() {
      base.Enter();
      _GameInstance = (_StateMachine.GetGame() as FollowCubeGame);
      _GameInstance.FailedAttempt();
      _CurrentRobot.SendAnimation(AnimationName.kShocked, HandleFailedAnimation);
    }


    private void HandleFailedAnimation(bool success) {
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      InitialCubesState initCubeState = new InitialCubesState();
      switch (_GameInstance.CurrentFollowTask) {
      case FollowCubeGame.FollowTask.Forwards:
        initCubeState.InitialCubeRequirements(new FollowCubeForwardState(), 1, true, null);
        break;
      case FollowCubeGame.FollowTask.Backwards:
        initCubeState.InitialCubeRequirements(new FollowCubeBackwardState(), 1, true, null);
        break;
      case FollowCubeGame.FollowTask.TurnLeft:
        initCubeState.InitialCubeRequirements(new FollowCubeTurnLeftState(), 1, true, null);
        break;
      case FollowCubeGame.FollowTask.TurnRight:
        initCubeState.InitialCubeRequirements(new FollowCubeTurnRightState(), 1, true, null);
        break;
      case FollowCubeGame.FollowTask.FollowDrive:
        initCubeState.InitialCubeRequirements(new FollowCubeDriveState(), 1, true, null);
        break;
      }

      _StateMachine.SetNextState(initCubeState);
    }

  }
}