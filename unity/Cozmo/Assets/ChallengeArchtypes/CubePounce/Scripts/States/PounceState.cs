using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace CubePounce {
  public class PounceState : State {

    private CubePounceGame _CubePounceGame;
    private float _AttemptDelay;
    private float _FirstTimestamp = -1;
    //private float _LastSeenTimeStamp = -1;
    public bool _AttemptTriggered = false;
    public bool _DidPounce = false;

    public override void Enter() {
      base.Enter();
      _CubePounceGame = (_StateMachine.GetGame() as CubePounceGame);
      GameAudioClient.SetMusicState(_CubePounceGame.GetDefaultMusicState());
      _AttemptDelay = _CubePounceGame.GetAttemptDelay();
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _CurrentRobot.SetLiftHeight(1.0f);
      _FirstTimestamp = Time.time;
      _AttemptTriggered = false;
      _DidPounce = false;
      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
      LightCube.OnMovedAction += HandleCubeMoved;
    }

    public override void Update() {
      base.Update();
      if (!_AttemptTriggered) {
        if (Time.time - _FirstTimestamp > _AttemptDelay) {
          _DidPounce = _CubePounceGame.AttemptPounce();
          _AttemptTriggered = true;
        }
      }
    }

    public override void Pause() {
      if (_DidPounce) {
        // Do nothing
      }
      else {
        base.Pause();
      }
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if ((!_AttemptTriggered || !_DidPounce) && id == _CubePounceGame.GetCurrentTarget().ID) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinEarly);
        _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinEarly);
        _CubePounceGame.OnCozmoWin();

        // Disable listener
        LightCube.OnMovedAction -= HandleCubeMoved;
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.OnMovedAction -= HandleCubeMoved;
    }
  }
}

