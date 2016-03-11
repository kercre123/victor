using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private const float _kArriveAtCubeThreshold = 50.0f;
    private const float _kTargetDistanceToCube = 10.0f;

    private SpeedTapGame _SpeedTapGame = null;

    private bool _IsFirstTime = false;

    public SpeedTapWaitForCubePlace(bool isFirstTime) {
      _IsFirstTime = isFirstTime;
    }

    private bool _GotoObjectComplete = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
      _CurrentRobot.SetEnableCliffSensor(true);
      if (_IsFirstTime) {
        _SpeedTapGame.InitialCubesDone();
      }
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      // Just hold on this state if all rounds are over.
      if (_SpeedTapGame.AllRoundsOver) {
        return;
      }
      _GotoObjectComplete = false;

      _CurrentRobot.SetLiftHeight(1.0f, HandleLiftRaiseComplete);
    }

    private void HandleLiftRaiseComplete(bool success) {
      if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude > _kTargetDistanceToCube) {
        _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, _kTargetDistanceToCube, HandleGotoObjectComplete);
      }
      else {
        _GotoObjectComplete = true;
      }
    }

    private void HandleGotoObjectComplete(bool success) {
      _GotoObjectComplete = true;
    }

    public override void Update() {
      if (_GotoObjectComplete) {
        if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude < _kArriveAtCubeThreshold) {
          // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
          _CurrentRobot.SetEnableCliffSensor(false);
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
