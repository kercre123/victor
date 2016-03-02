using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private const float _kArriveAtCubeThreshold = 90.0f;
    private const float _kTargetDistanceToCube = 70.0f;

    private SpeedTapGame _SpeedTapGame = null;

    private bool _ShowHowToPlay = false;

    public SpeedTapWaitForCubePlace(bool showHowToPlay) {
      _ShowHowToPlay = showHowToPlay;
    }

    private bool _GotoObjectComplete = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      if (_ShowHowToPlay) {
        _SpeedTapGame.InitialCubesDone();
      }
      // TODO: Set up UI
      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      // Just hold on this state if all rounds are over.
      if (_SpeedTapGame.AllRoundsOver) {
        return;
      }
      _GotoObjectComplete = false;
      _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, _kTargetDistanceToCube, HandleGotoObjectComplete);

      if (_ShowHowToPlay) {
        _StateMachine.PushSubState(new HowToPlayState(null));
      }
    }

    private void HandleGotoObjectComplete(bool success) {
      _GotoObjectComplete = true;
    }

    public override void Update() {
      if (_GotoObjectComplete) {
        if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude < _kArriveAtCubeThreshold) {
          _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
        }
        else {
          // restart this state
          _SpeedTapGame.InitialCubesDone();
          _StateMachine.SetNextState(new SpeedTapWaitForCubePlace(false));
        }
      }
    }
  }

}
