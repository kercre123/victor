using UnityEngine;
using System.Collections;

namespace Playhouse {
  public class RequestPlay : State {

    private PlayhouseGame _GameInstance;

    public override void Enter() {
      base.Enter();
      RobotEngineManager.Instance.CurrentRobot.SendAnimation(AnimationName.kEnjoyPattern, AnimationDone);
      _GameInstance = _StateMachine.GetGame() as PlayhouseGame;
    }

    private void AnimationDone(bool success) {
      _GameInstance.RequestAnimationDone();
    }

  }

}
