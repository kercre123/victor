using UnityEngine;
using System.Collections;

namespace CubePounce {
  public class PounceState : State {

    private CubePounceGame _CubeSlapGame;
    private float _SlapDelay;
    private float _FirstTimestamp = -1;
    //private float _LastSeenTimeStamp = -1;
    public bool _SlapTriggered = false;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubePounceGame);
      _SlapDelay = _CubeSlapGame.GetSlapDelay();
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _CurrentRobot.SetLiftHeight(1.0f);
      _FirstTimestamp = Time.time;
      _SlapTriggered = false;
      _CubeSlapGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubeSlapGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
      LightCube.OnMovedAction += HandleCubeMoved;
    }

    public override void Update() {
      base.Update();
      if (!_SlapTriggered) {
        if (Time.time - _FirstTimestamp > _SlapDelay) {
          _CubeSlapGame.AttemptPounce();
          _SlapTriggered = true;
        }
      }
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (!_SlapTriggered && id == _CubeSlapGame.GetCurrentTarget().ID) {
        _CubeSlapGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinEarly);
        _CubeSlapGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinEarly);
        _CubeSlapGame.OnCozmoWin();
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.OnMovedAction -= HandleCubeMoved;
    }
  }
}

