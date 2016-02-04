using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      // TODO: Set up UI

      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.ResetScore();

      _CurrentRobot.GotoObject(_SpeedTapGame.CozmoBlock, 60f, HandleGotoObjectComplete);
    }

    private void HandleGotoObjectComplete(bool success) {

      if((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude < 80f) {
        _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
      }
      else {
        // restart this state
        _StateMachine.SetNextState(new SpeedTapWaitForCubePlace());
      }
    }

    public override void Exit() {
      base.Exit();
      // set idle parameters
      /*Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
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
      _CurrentRobot.SetLiveIdleAnimationParameters(paramNames, paramValues);*/
    }
  }

}
