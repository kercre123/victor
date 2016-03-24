using UnityEngine;
using System.Collections;

namespace CubePounce {
  public class PounceState : State {

    private CubePounceGame _CubePounceGame;
    private float _PounceDelay;
    private float _FirstTimestamp = -1;
    //private float _LastSeenTimeStamp = -1;
    public bool _PounceTriggered = false;

    public override void Enter() {
      base.Enter();
      _CubePounceGame = (_StateMachine.GetGame() as CubePounceGame);
      _PounceDelay = _CubePounceGame.GetSlapDelay();
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _CurrentRobot.SetLiftHeight(1.0f);
      _FirstTimestamp = Time.time;
      _PounceTriggered = false;
      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
      LightCube.OnMovedAction += HandleCubeMoved;
    }

    public override void Update() {
      base.Update();
      if (!_PounceTriggered) {
        if (Time.time - _FirstTimestamp > _PounceDelay) {
          _CubePounceGame.AttemptPounce();
          _PounceTriggered = true;
        }
      }
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (!_PounceTriggered && id == _CubePounceGame.GetCurrentTarget().ID) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinEarly);
        _CubePounceGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinEarly);
        _CubePounceGame.OnCozmoWin();
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.OnMovedAction -= HandleCubeMoved;
    }
  }
}

