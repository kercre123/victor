using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private SpeedTapGame _SpeedTapGame = null;

    private bool _ShowHowToPlay = false;

    public SpeedTapWaitForCubePlace(bool showHowToPlay) {
      _ShowHowToPlay = showHowToPlay;
    }

    private bool _GotoObjectComplete = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      // TODO: Set up UI

      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);

      _CurrentRobot.GotoObject(_SpeedTapGame.CozmoBlock, 60f, HandleGotoObjectComplete);

      if (_ShowHowToPlay) {
        _StateMachine.PushSubState(new HowToPlayState(null));
      }
    }

    private void HandleGotoObjectComplete(bool success) {
      _GotoObjectComplete = true;
    }

    public override void Update() {
      if (_GotoObjectComplete) {
        if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude < 80f) {
          _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
        }
        else {
          // restart this state
          _StateMachine.SetNextState(new SpeedTapWaitForCubePlace(false));
        }
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
