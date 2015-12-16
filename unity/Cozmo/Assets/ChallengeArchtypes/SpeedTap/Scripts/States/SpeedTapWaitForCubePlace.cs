using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
    }

    public override void Update() {
      base.Update();
      if (_SpeedTapGame.CozmoBlock.MarkersVisible) {
        float distance = Vector3.Distance(_CurrentRobot.WorldPosition, _SpeedTapGame.CozmoBlock.WorldPosition);
        if (distance < 60.0f) {
          _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
        }
      }
    }

    public override void Exit() {
      base.Exit();
      // set idle parameters
      Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
        Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementSpacingMin_ms,
        Anki.Cozmo.LiveIdleAnimationParameter.LiftMovementSpacingMin_ms,
        Anki.Cozmo.LiveIdleAnimationParameter.HeadAngleVariability_deg,
      };

      float[] paramValues = {
        float.MaxValue,
        float.MaxValue,
        20.0f
      };
      _CurrentRobot.SetIdleAnimation("_LIVE_");
      _CurrentRobot.SetLiveIdleAnimationParameters(paramNames, paramValues);
    }
  }

}
