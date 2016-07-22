using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateFakeOut : CubePounceState {
    
    private float _FakeOutDelay_s;
    private float _FirstTimestamp = -1;
    public bool _FakeOutTriggered = false;

    public override void Enter() {
      base.Enter();

      GameAudioClient.SetMusicState(_CubePounceGame.GetDefaultMusicState());
      _FakeOutDelay_s = _CubePounceGame.GetAttemptDelay();
      _FirstTimestamp = Time.time;

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
    }

    public override void Update() {
      base.Update();

      if (!_CubePounceGame.CubeSeenRecently || !_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistLoose_mm)) {
        _StateMachine.SetNextState(new CubePounceStateResetPoint());
        return;
      }

      if (!_FakeOutTriggered && (Time.time - _FirstTimestamp) > _FakeOutDelay_s) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceFake, HandleFakeoutEnd);
        _CubePounceGame.IncreasePounceChance();
        _FakeOutTriggered = true;
      }
    }

    private void HandleFakeoutEnd(bool success) {
      _StateMachine.SetNextState(_CubePounceGame.GetNextFakeoutOrAttemptState());
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleFakeoutEnd);
    }
  }
}
