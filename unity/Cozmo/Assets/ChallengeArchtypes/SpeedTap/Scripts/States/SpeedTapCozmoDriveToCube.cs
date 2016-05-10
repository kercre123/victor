using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapCozmoDriveToCube : State {

    private const float _kArriveAtCubeThreshold = 60.0f;
    private const float _kTargetDistanceToCube = 45.0f;

    private SpeedTapGame _SpeedTapGame = null;

    private bool _IsFirstTime = false;

    public SpeedTapCozmoDriveToCube(bool isFirstTime) {
      _IsFirstTime = isFirstTime;
    }

    private bool _GotoObjectComplete = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      if (_IsFirstTime) {
        _SpeedTapGame.InitialCubesDone();
      }
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _SpeedTapGame.StartCycleCube(_SpeedTapGame.CozmoBlock, 
        Cozmo.CubePalette.TapMeColor.lightColors, 
        Cozmo.CubePalette.TapMeColor.cycleIntervalSeconds);

      _SpeedTapGame.ShowWaitForCozmoSlide();
      _GotoObjectComplete = false;

      _CurrentRobot.SetLiftHeight(1.0f, HandleLiftRaiseComplete);
    }

    private void HandleLiftRaiseComplete(bool success) {
      if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude > _kTargetDistanceToCube) {
        _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, 0.0f, HandleGotoObjectComplete, false, true);
      }
      else {
        _GotoObjectComplete = true;
      }
    }

    private void HandleGotoObjectComplete(bool success) {
      _GotoObjectComplete = success;
      if (success == false) {
        _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, 0.0f, HandleGotoObjectComplete, false, true);
      }
    }

    public override void Update() {
      if (_GotoObjectComplete) {
        if ((_CurrentRobot.WorldPosition - _SpeedTapGame.CozmoBlock.WorldPosition).magnitude < _kArriveAtCubeThreshold) {
          _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
        }
        else {
          _StateMachine.SetNextState(new SpeedTapCozmoDriveToCube(false));
        }
      }
    }
  }

}
